#include <gtest/gtest.h>

#include <openssl/evp.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "update/update_manifest.h"
#include "update/update_url_policy.h"
#include "update/redirect_chain_controller.h"
#include "update/update_package_verifier.h"
#include "update/platform_package_verifier.h"
#include "update/file_update_rollback_store.h"
#include "update/installation_gate.h"
#include "update/linux_appimage_gate.h"
#include "update/prepared_appimage.h"
#include "update/update_rollback_store.h"

namespace {
using namespace gdl::update;

struct Ed25519Key {
	EVP_PKEY* key{nullptr};
	~Ed25519Key() { EVP_PKEY_free(key); }
};

Ed25519Key GenerateKey() {
	Ed25519Key result;
	auto* context = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
	EXPECT_NE(context, nullptr);
	EXPECT_EQ(EVP_PKEY_keygen_init(context), 1);
	EXPECT_EQ(EVP_PKEY_keygen(context, &result.key), 1);
	EVP_PKEY_CTX_free(context);
	return result;
}

std::string Base64(const unsigned char* data, size_t size) {
	std::string encoded(4 * ((size + 2) / 3), '\0');
	const int written = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(encoded.data()), data,
		static_cast<int>(size));
	encoded.resize(written);
	return encoded;
}

std::string PublicKeyBase64(EVP_PKEY* key) {
	unsigned char raw[32];
	size_t size = sizeof(raw);
	EXPECT_EQ(EVP_PKEY_get_raw_public_key(key, raw, &size), 1);
	return Base64(raw, size);
}

std::string Sign(EVP_PKEY* key, const std::string& message) {
	auto* context = EVP_MD_CTX_new();
	EXPECT_EQ(EVP_DigestSignInit(context, nullptr, nullptr, nullptr, key), 1);
	size_t size = 0;
	EXPECT_EQ(EVP_DigestSign(context, nullptr, &size,
		reinterpret_cast<const unsigned char*>(message.data()), message.size()), 1);
	std::vector<unsigned char> signature(size);
	EXPECT_EQ(EVP_DigestSign(context, signature.data(), &size,
		reinterpret_cast<const unsigned char*>(message.data()), message.size()), 1);
	EVP_MD_CTX_free(context);
	return Base64(signature.data(), size);
}

nlohmann::json ManifestJson() {
	return {{"schema_version", 1}, {"release_id", 42}, {"version", "1.2.3"},
		{"platform", "windows-x64"}, {"published_at", 1000}, {"expires_at", 2000},
		{"notes", "修复更新"}, {"asset", {{"name", "GDownload-1.2.3.exe"},
			{"url", "https://github.com/cool2528/GDownload/releases/download/v1.2.3/GDownload.exe"},
			{"size", 4}, {"sha256", std::string(64, 'a')}}}};
}

TEST(UpdateManifestTest, CanonicalJsonIsCompactSortedUnicodeAndOmitsSignature) {
	auto json = ManifestJson();
	json["signature"] = "ignored";
	const auto canonical = CanonicalizeManifest(json.dump());
	ASSERT_TRUE(canonical.has_value());
	EXPECT_EQ(canonical->find("signature"), std::string::npos);
	EXPECT_EQ(canonical->find("修复更新") != std::string::npos, true);
	EXPECT_EQ(*canonical, CanonicalizeManifest(nlohmann::json::parse(json.dump()).dump(2)).value());
}

TEST(UpdateManifestTest, VerifiesSignatureAndPolicyAndRejectsTampering) {
	auto key = GenerateKey();
	auto json = ManifestJson();
	json["signature"] = Sign(key.key, CanonicalizeManifest(json.dump()).value());
	ManifestPolicy policy{"windows-x64", ".exe", {"github.com"}, 41, 1500};
	auto valid = VerifyUpdateManifest(json.dump(), PublicKeyBase64(key.key), policy);
	ASSERT_TRUE(valid.ok) << valid.error;
	EXPECT_EQ(valid.manifest.release_id, 42);
	json["asset"]["size"] = 5;
	EXPECT_FALSE(VerifyUpdateManifest(json.dump(), PublicKeyBase64(key.key), policy).ok);
}

TEST(UpdateManifestTest, EmptyTrustedKeyFailsClosed) {
	ManifestPolicy policy{"windows-x64", ".exe", {"github.com"}, 0, 1500};
	const auto result = VerifyUpdateManifest(ManifestJson().dump(), "", policy);
	EXPECT_FALSE(result.ok);
	EXPECT_NE(result.error.find("public key"), std::string::npos);
}

TEST(UpdateManifestTest, RejectsMalformedExpiredRollbackPlatformHostSchemeSuffixAndDigest) {
	auto key = GenerateKey();
	auto check = [&](nlohmann::json json, ManifestPolicy policy) {
		json["signature"] = Sign(key.key, CanonicalizeManifest(json.dump()).value());
		return VerifyUpdateManifest(json.dump(), PublicKeyBase64(key.key), policy).ok;
	};
	ManifestPolicy policy{"windows-x64", ".exe", {"github.com"}, 41, 1500};
	EXPECT_FALSE(VerifyUpdateManifest("{", PublicKeyBase64(key.key), policy).ok);
	auto json = ManifestJson(); json["expires_at"] = 1400; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); policy.highest_release_id = 43; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); policy.highest_release_id = 42; EXPECT_TRUE(check(json, policy));
	policy.highest_release_id = 41; json["platform"] = "linux-x64"; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); json["asset"]["url"] = "http://github.com/a.exe"; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); json["asset"]["url"] = "https://evil.example/a.exe"; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); json["asset"]["name"] = "a.zip"; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); json["asset"]["sha256"] = "bad"; EXPECT_FALSE(check(json, policy));
}

TEST(UpdateManifestTest, RejectsNonDefaultHttpsPort) {
	auto key = GenerateKey();
	auto json = ManifestJson();
	json["asset"]["url"] = "https://github.com:444/cool2528/GDownload/releases/download/v1.2.3/GDownload.exe";
	json["signature"] = Sign(key.key, CanonicalizeManifest(json.dump()).value());
	ManifestPolicy policy{"windows-x64", ".exe", {"github.com"}, 41, 1500};

	EXPECT_FALSE(VerifyUpdateManifest(json.dump(), PublicKeyBase64(key.key), policy).ok);
}

TEST(UpdateUrlPolicyTest, AcceptsOnlyExactAllowedHttpsHosts) {
	const std::vector<std::string> allowed_hosts{"github.com"};

	EXPECT_TRUE(ValidateDownloadUrl("https://github.com/release.exe", allowed_hosts));
	EXPECT_TRUE(ValidateDownloadUrl("HTTPS://GITHUB.COM:443/release.exe", allowed_hosts));
	EXPECT_FALSE(ValidateDownloadUrl("http://github.com/release.exe", allowed_hosts));
	EXPECT_FALSE(ValidateDownloadUrl("https://github.com.evil.example/release.exe", allowed_hosts));
	EXPECT_FALSE(ValidateDownloadUrl("https://evil.example/release.exe", allowed_hosts));
}

TEST(UpdateUrlPolicyTest, RejectsUserinfoAndNonDefaultPorts) {
	const std::vector<std::string> allowed_hosts{"github.com"};

	EXPECT_FALSE(ValidateDownloadUrl("https://github.com@evil.example/release.exe", allowed_hosts));
	EXPECT_FALSE(ValidateDownloadUrl("https://evil.example@github.com/release.exe", allowed_hosts));
	EXPECT_FALSE(ValidateDownloadUrl("https://github.com:444/release.exe", allowed_hosts));
	EXPECT_TRUE(ValidateDownloadUrl("https://github.com:/release.exe", allowed_hosts));
}

TEST(UpdateUrlPolicyTest, RedirectDecisionRejectsUntrustedTargets) {
	const std::vector<std::string> allowed_hosts{"github.com"};

	EXPECT_EQ(DecideDownloadRedirect("https://github.com:443/next.exe", allowed_hosts),
		RedirectDecision::kFollow);
	EXPECT_EQ(DecideDownloadRedirect("https://evil.example/payload.exe", allowed_hosts),
		RedirectDecision::kReject);
	EXPECT_EQ(DecideDownloadRedirect("https://github.com@evil.example/payload.exe", allowed_hosts),
		RedirectDecision::kReject);
}

TEST(RedirectChainControllerTest, ResolvesRelativeRedirectAndValidatesTarget) {
	RedirectChainController chain("https://github.com/releases/latest/app.exe", {"github.com"});
	const auto result = chain.Follow("../download/app.exe");
	ASSERT_EQ(result.decision, RedirectChainDecision::kFollow);
	EXPECT_EQ(result.url, "https://github.com/releases/download/app.exe");
}

TEST(RedirectChainControllerTest, RejectsUntrustedAndMalformedRedirects) {
	RedirectChainController chain("https://github.com/releases/app.exe", {"github.com"});
	EXPECT_EQ(chain.Follow("https://evil.example/app.exe").decision, RedirectChainDecision::kReject);
	EXPECT_EQ(chain.Follow("http://github.com/app.exe").decision, RedirectChainDecision::kReject);
}

TEST(RedirectChainControllerTest, RejectsRedirectLoops) {
	RedirectChainController chain("https://github.com/a", {"github.com"});
	ASSERT_EQ(chain.Follow("/b").decision, RedirectChainDecision::kFollow);
	EXPECT_EQ(chain.Follow("/a").decision, RedirectChainDecision::kLoop);
}

TEST(RedirectChainControllerTest, AllowsAtMostFiveRedirects) {
	RedirectChainController chain("https://github.com/0", {"github.com"});
	for (int index = 1; index <= 5; ++index) {
		EXPECT_EQ(chain.Follow("/" + std::to_string(index)).decision, RedirectChainDecision::kFollow);
	}
	EXPECT_EQ(chain.Follow("/6").decision, RedirectChainDecision::kTooManyRedirects);
}

TEST(UpdatePackageVerifierTest, ParsesStrictGithubDigest) {
	const auto hex = std::string(64, 'a');
	EXPECT_EQ(ParseGithubSha256Digest("sha256:" + hex), hex);
	EXPECT_FALSE(ParseGithubSha256Digest(hex).has_value());
	EXPECT_FALSE(ParseGithubSha256Digest("sha256:" + std::string(63, 'a')).has_value());
}

TEST(UpdatePackageVerifierTest, DetectsSameSizeTamperingAndMissingFiles) {
	const auto dir = std::filesystem::temp_directory_path() / "gdownload-update-trust";
	std::filesystem::create_directories(dir);
	const auto file = dir / "package.exe";
	{ std::ofstream out(file, std::ios::binary); out << "good"; }
	const auto digest = ComputeFileSha256(file);
	ASSERT_TRUE(digest.has_value());
	EXPECT_TRUE(VerifyUpdatePackage(file, 4, *digest).ok);
	{ std::ofstream out(file, std::ios::binary | std::ios::trunc); out << "evil"; }
	EXPECT_FALSE(VerifyUpdatePackage(file, 4, *digest).ok);
	EXPECT_FALSE(VerifyUpdatePackage(dir / "missing.exe", 4, *digest).ok);
	std::filesystem::remove_all(dir);
}

TEST(UpdatePackageVerifierTest, TemporaryPathsAreUnique) {
	const auto a = CreateUniqueUpdateTempPath(std::filesystem::temp_directory_path(), ".exe");
	const auto b = CreateUniqueUpdateTempPath(std::filesystem::temp_directory_path(), ".exe");
	EXPECT_NE(a, b);
	EXPECT_EQ(a.extension(), ".exe");
}

TEST(PlatformPackageVerifierTest, SignerPinIsStrictAndFailClosed) {
	const auto hex = std::string(64, 'b');
	EXPECT_TRUE(SignerPinMatches("sha256:" + hex, hex));
	EXPECT_FALSE(SignerPinMatches({}, hex));
	EXPECT_FALSE(SignerPinMatches("sha256:" + std::string(64, 'a'), hex));
	EXPECT_FALSE(SignerPinMatches(hex, hex));
}

// 本项目为开源免费签名方案(ed25519 更新清单 + AppImage GPG),不购买付费 Windows Authenticode 证书,
// 因此发布流水线不会再注入 GDOWNLOAD_UPDATE_SIGNER_SPKI_PIN,expected_signer_pin 恒为空字符串。
// 下面两个用例验证跳过逻辑落在 WinAuthenticodeVerifier::Verify 这一层(而非 SignerPinMatches 本身，
// 后者作为纯字符串比对函数继续严格 fail-closed，见上面的用例)：
// 空 pin 时整层 Authenticode 校验被跳过并放行；非空 pin 时未签名的包仍应被严格拒绝。
#ifdef _WIN32
TEST(PlatformPackageVerifierTest, EmptyPinSkipsAuthenticodeCheckAndAcceptsUnsignedPackage) {
	const auto dir = std::filesystem::temp_directory_path() / "gdownload-platform-verifier-empty-pin";
	std::filesystem::create_directories(dir);
	const auto package = dir / "package.exe";
	{ std::ofstream out(package, std::ios::binary); out << "unsigned package that already passed ed25519+sha256"; }

	const auto verifier = CreatePlatformPackageVerifier();
	ASSERT_NE(verifier, nullptr);
	// 该包未经任何 Authenticode 签名；expected_signer_pin 为空时应跳过该层校验直接放行，
	// 而不是像旧的 fail-closed 逻辑那样因“pin 未配置”或“找不到签名”而拒绝。
	const auto result = verifier->Verify(package, "");
	EXPECT_TRUE(result.ok) << result.error;

	std::filesystem::remove_all(dir);
}

TEST(PlatformPackageVerifierTest, NonEmptyPinStillFailsClosedOnUnsignedPackage) {
	const auto dir = std::filesystem::temp_directory_path() / "gdownload-platform-verifier-nonempty-pin";
	std::filesystem::create_directories(dir);
	const auto package = dir / "package.exe";
	{ std::ofstream out(package, std::ios::binary); out << "unsigned package"; }

	const auto verifier = CreatePlatformPackageVerifier();
	ASSERT_NE(verifier, nullptr);
	// 一旦配置了非空 pin(意味着拥有证书、启用了 Authenticode 签名)，未签名的包必须继续被拒绝，
	// 证明本次改动只放开了“pin 为空”这一种情况，其余场景严格性不变。
	const auto result = verifier->Verify(package, "sha256:" + std::string(64, 'c'));
	EXPECT_FALSE(result.ok);

	std::filesystem::remove_all(dir);
}
#endif

class FakePlatformPackageVerifier final : public IPlatformPackageVerifier {
   public:
	PackageVerificationResult Verify(const std::filesystem::path& package,
		const std::string& expected_signer_pin) const override {
		++verify_count;
		last_package = package;
		last_signer_pin = expected_signer_pin;
		return result;
	}

	mutable int verify_count{0};
	mutable std::filesystem::path last_package;
	mutable std::string last_signer_pin;
	PackageVerificationResult result{true, {}};
};

class FakeInstallerLauncher final : public IInstallerLauncher {
   public:
	InstallationResult LaunchAndWait(const std::filesystem::path& package, std::stop_token) override {
		++launch_count;
		last_package = package;
		if (on_launch) on_launch();
		return result;
	}

	int launch_count{0};
	std::filesystem::path last_package;
	InstallationResult result{InstallationStatus::kSucceeded, 0, {}};
	std::function<void()> on_launch;
};

class FakeUpdateRollbackStore final : public IUpdateRollbackStore {
   public:
	RollbackPersistenceResult AcquireInstallationLease() override { return lease_result; }
	void ReleaseInstallationLease() override { ++release_count; }
	RollbackReadResult HighestReleaseId() const override {
		const auto index = read_count++;
		if (index < read_results.size()) return read_results[index];
		return {true, highest_release_id, {}};
	}
	RollbackPersistenceResult PersistHighestReleaseId(std::uint64_t release_id) override {
		++persist_count;
		highest_release_id = release_id;
		return persist_result;
	}
	RollbackPersistenceResult RestoreHighestReleaseId(std::uint64_t release_id) override {
		++restore_count; highest_release_id = release_id; return restore_result;
	}

	std::uint64_t highest_release_id{0};
	int persist_count{0};
	RollbackPersistenceResult persist_result{true, {}};
	RollbackPersistenceResult lease_result{true, {}};
	int release_count{0};
	mutable std::size_t read_count{0};
	std::vector<RollbackReadResult> read_results;
	int restore_count{0};
	RollbackPersistenceResult restore_result{true, {}};
};

class InstallationGateTest : public testing::Test {
   protected:
	void SetUp() override {
		directory = std::filesystem::temp_directory_path() / "gdownload-installation-gate";
		std::filesystem::create_directories(directory);
		package = directory / "package.exe";
		{
			std::ofstream out(package, std::ios::binary);
			out << "good";
		}
		digest = ComputeFileSha256(package).value();
	}

	void TearDown() override { std::filesystem::remove_all(directory); }

	std::filesystem::path directory;
	std::filesystem::path package;
	std::string digest;
	FakePlatformPackageVerifier platform_verifier;
	FakeInstallerLauncher launcher;
	FakeUpdateRollbackStore rollback_store;
};

TEST_F(InstallationGateTest, SuccessfulInstallationLaunchesOnceAndPersistsReleaseId) {
	InstallationGate gate(platform_verifier, launcher, rollback_store);

	EXPECT_TRUE(gate.Install(package, 4, digest, "sha256:signer", 42).Succeeded());
	EXPECT_EQ(platform_verifier.verify_count, 1);
	EXPECT_EQ(launcher.launch_count, 1);
	EXPECT_EQ(rollback_store.persist_count, 1);
	EXPECT_EQ(rollback_store.HighestReleaseId().value, 42);
}

TEST_F(InstallationGateTest, PlatformVerificationFailureDoesNotLaunchOrPersist) {
	platform_verifier.result = {false, "untrusted signer"};
	InstallationGate gate(platform_verifier, launcher, rollback_store);

	EXPECT_FALSE(gate.Install(package, 4, digest, "sha256:signer", 42).Succeeded());
	EXPECT_EQ(launcher.launch_count, 0);
	EXPECT_EQ(rollback_store.persist_count, 0);
}

TEST_F(InstallationGateTest, LauncherFailureDoesNotPersist) {
	launcher.result = {InstallationStatus::kLaunchFailed, 0, "failed to start"};
	InstallationGate gate(platform_verifier, launcher, rollback_store);

	EXPECT_FALSE(gate.Install(package, 4, digest, "sha256:signer", 42).Succeeded());
	EXPECT_EQ(launcher.launch_count, 1);
	EXPECT_EQ(rollback_store.persist_count, 0);
}

TEST_F(InstallationGateTest, NonZeroExitAndTimeoutDoNotPersist) {
	InstallationGate gate(platform_verifier, launcher, rollback_store);
	launcher.result = {InstallationStatus::kCancelledOrNonZero, 7, "non-zero exit"};
	EXPECT_FALSE(gate.Install(package, 4, digest, "sha256:signer", 42).Succeeded());
	EXPECT_EQ(rollback_store.persist_count, 0);
	launcher.result = {InstallationStatus::kTimedOut, 0, "timed out"};
	EXPECT_FALSE(gate.Install(package, 4, digest, "sha256:signer", 42).Succeeded());
	EXPECT_EQ(rollback_store.persist_count, 0);
}

TEST_F(InstallationGateTest, ConcurrentHigherReleasePreventsLaunch) {
	rollback_store.highest_release_id = 50;
	InstallationGate gate(platform_verifier, launcher, rollback_store);
	const auto result = gate.Install(package, 4, digest, "sha256:signer", 42);
	EXPECT_EQ(result.status, InstallationStatus::kRollbackRejected);
	EXPECT_EQ(launcher.launch_count, 0);
}

TEST_F(InstallationGateTest, HigherReleaseAppearingDuringInstallIsNotLowered) {
	launcher.on_launch = [&] { rollback_store.highest_release_id = 100; };
	InstallationGate gate(platform_verifier, launcher, rollback_store);
	EXPECT_TRUE(gate.Install(package, 4, digest, "sha256:signer", 42).Succeeded());
	EXPECT_EQ(rollback_store.HighestReleaseId().value, 100);
}

TEST_F(InstallationGateTest, PersistenceFailureIsVisible) {
	rollback_store.persist_result = {false, "sync failed"};
	InstallationGate gate(platform_verifier, launcher, rollback_store);
	const auto result = gate.Install(package, 4, digest, "sha256:signer", 42);
	EXPECT_EQ(result.status, InstallationStatus::kPersistenceFailed);
	EXPECT_FALSE(result.Succeeded());
}

TEST_F(InstallationGateTest, LeaseAcquisitionFailurePreventsLaunch) {
	rollback_store.lease_result = {false, "locked"};
	InstallationGate gate(platform_verifier, launcher, rollback_store);
	const auto result = gate.Install(package, 4, digest, "sha256:signer", 42);
	EXPECT_EQ(result.status, InstallationStatus::kPersistenceFailed);
	EXPECT_EQ(launcher.launch_count, 0);
}

TEST_F(InstallationGateTest, RollbackPreReadFailurePreventsLaunch) {
	rollback_store.read_results = {{false, 0, "corrupt rollback state"}};
	InstallationGate gate(platform_verifier, launcher, rollback_store);
	const auto result = gate.Install(package, 4, digest, "sha256:signer", 42);
	EXPECT_EQ(result.status, InstallationStatus::kRollbackStateFailed);
	EXPECT_EQ(launcher.launch_count, 0);
	EXPECT_EQ(rollback_store.persist_count, 0);
}

TEST_F(InstallationGateTest, RollbackPostReadFailureIsVisibleAndDoesNotPersist) {
	rollback_store.read_results = {{true, 0, {}}, {false, 0, "rollback reread failed"}};
	InstallationGate gate(platform_verifier, launcher, rollback_store);
	const auto result = gate.Install(package, 4, digest, "sha256:signer", 42);
	EXPECT_EQ(result.status, InstallationStatus::kRollbackStateFailed);
	EXPECT_EQ(launcher.launch_count, 1);
	EXPECT_EQ(rollback_store.persist_count, 0);
}

TEST(FileUpdateRollbackStoreTest, StrictlyReadsAndAtomicallyPersistsMonotonicReleaseId) {
	const auto root = std::filesystem::temp_directory_path() / "gdownload-file-rollback-store";
	std::filesystem::remove_all(root);
	const auto path = root / "highest_release_id";
	FileUpdateRollbackStore store(path);
	EXPECT_EQ(store.HighestReleaseId().value, 0);
	ASSERT_TRUE(store.PersistHighestReleaseId(42).ok);
	EXPECT_EQ(store.HighestReleaseId().value, 42);
	ASSERT_TRUE(store.PersistHighestReleaseId(7).ok);
	EXPECT_EQ(store.HighestReleaseId().value, 42);
	{ std::ofstream output(path, std::ios::trunc); output << "42junk\n"; }
	EXPECT_FALSE(store.HighestReleaseId().ok);
	std::filesystem::remove_all(root);
}

TEST(FileUpdateRollbackStoreTest, DirectoryAtStatePathFailsClosed) {
	const auto root = std::filesystem::temp_directory_path() / "gdownload-file-rollback-store-dir";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root / "highest_release_id");
	FileUpdateRollbackStore store(root / "highest_release_id");
	EXPECT_FALSE(store.HighestReleaseId().ok);
	std::filesystem::remove_all(root);
}

class FakeAppImageVerifier final : public ILinuxAppImageVerifier {
   public:
	AppImageSignatureState Verify(const std::filesystem::path& path) const override {
		++calls; if (on_verify) on_verify(path); return state;
	}
	mutable int calls{0};
	AppImageSignatureState state{AppImageSignatureState::kPassed};
	std::function<void(const std::filesystem::path&)> on_verify;
};

class FakeAppImageLauncher final : public ILinuxAppImageLauncher {
   public:
	InstallationResult Launch(const std::filesystem::path&) override { ++calls; return result; }
	int calls{0};
	InstallationResult result{InstallationStatus::kSucceeded, 0, {}};
};

class LinuxAppImageGateTest : public InstallationGateTest {
   protected:
	FakeAppImageVerifier appimage_verifier;
	FakeAppImageLauncher appimage_launcher;
};

TEST_F(LinuxAppImageGateTest, HashFailureDoesNotVerifyOrLaunch) {
	LinuxAppImageGate gate(appimage_verifier, appimage_launcher, rollback_store, false);
	EXPECT_FALSE(gate.Apply(package, 4, std::string(64, '0'), 42).Succeeded());
	EXPECT_EQ(appimage_verifier.calls, 0); EXPECT_EQ(appimage_launcher.calls, 0);
}

TEST_F(LinuxAppImageGateTest, InvalidAndKeyChangedNeverLaunch) {
	LinuxAppImageGate gate(appimage_verifier, appimage_launcher, rollback_store, false);
	appimage_verifier.state = AppImageSignatureState::kInvalid;
	EXPECT_FALSE(gate.Apply(package, 4, digest, 42).Succeeded());
	appimage_verifier.state = AppImageSignatureState::kKeyChanged;
	EXPECT_FALSE(gate.Apply(package, 4, digest, 42).Succeeded());
	EXPECT_EQ(appimage_launcher.calls, 0);
}

TEST_F(LinuxAppImageGateTest, UnsignedAllowedOnlyDuringMigration) {
	appimage_verifier.state = AppImageSignatureState::kUnsigned;
	LinuxAppImageGate migration_gate(appimage_verifier, appimage_launcher, rollback_store, false);
	EXPECT_TRUE(migration_gate.Apply(package, 4, digest, 42).Succeeded());
	appimage_launcher.calls = 0;
	rollback_store.highest_release_id = 0;
	LinuxAppImageGate strict_gate(appimage_verifier, appimage_launcher, rollback_store, true);
	EXPECT_FALSE(strict_gate.Apply(package, 4, digest, 42).Succeeded());
	EXPECT_EQ(appimage_launcher.calls, 0);
}

TEST_F(LinuxAppImageGateTest, PassedSignatureLaunches) {
	LinuxAppImageGate gate(appimage_verifier, appimage_launcher, rollback_store, true);
	EXPECT_TRUE(gate.Apply(package, 4, digest, 42).Succeeded());
	EXPECT_EQ(appimage_launcher.calls, 1);
}

TEST_F(LinuxAppImageGateTest, RollbackReadFailureAndOldReleaseDoNotLaunch) {
	rollback_store.read_results = {{false, 0, "corrupt"}};
	LinuxAppImageGate gate(appimage_verifier, appimage_launcher, rollback_store, true);
	EXPECT_EQ(gate.Apply(package, 4, digest, 42).status, InstallationStatus::kRollbackStateFailed);
	EXPECT_EQ(appimage_launcher.calls, 0);
	rollback_store.read_results.clear(); rollback_store.read_count = 0; rollback_store.highest_release_id = 42;
	EXPECT_EQ(gate.Apply(package, 4, digest, 42).status, InstallationStatus::kRollbackRejected);
	EXPECT_EQ(appimage_launcher.calls, 0);
}

TEST_F(LinuxAppImageGateTest, PersistFailureIsVisibleAndPreventsHandoff) {
	rollback_store.persist_result = {false, "sync failed"};
	LinuxAppImageGate gate(appimage_verifier, appimage_launcher, rollback_store, true);
	EXPECT_EQ(gate.Apply(package, 4, digest, 42).status, InstallationStatus::kPersistenceFailed);
	EXPECT_EQ(appimage_launcher.calls, 0);
}

TEST_F(LinuxAppImageGateTest, LaunchFailureRestoresPreviousReleaseExactly) {
	rollback_store.highest_release_id = 7;
	appimage_launcher.result = {InstallationStatus::kLaunchFailed, 0, "handoff failed"};
	LinuxAppImageGate gate(appimage_verifier, appimage_launcher, rollback_store, true);
	EXPECT_EQ(gate.Apply(package, 4, digest, 42).status, InstallationStatus::kLaunchFailed);
	EXPECT_EQ(rollback_store.highest_release_id, 7);
	EXPECT_EQ(rollback_store.restore_count, 1);
}

TEST_F(LinuxAppImageGateTest, StagedReplacementDuringVerificationPreventsLaunch) {
	appimage_verifier.on_verify = [](const std::filesystem::path& path) {
		std::filesystem::remove(path);
		std::ofstream output(path, std::ios::binary); output << "evil";
	};
	LinuxAppImageGate gate(appimage_verifier, appimage_launcher, rollback_store, true);
	EXPECT_EQ(gate.Apply(package, 4, digest, 42).status, InstallationStatus::kVerificationFailed);
	EXPECT_EQ(appimage_launcher.calls, 0);
}

TEST(PreparedAppImageTest, SourceReplacementDoesNotChangePrivateStagedCopy) {
	const auto root = std::filesystem::temp_directory_path() / "gdownload-prepared-appimage";
	std::filesystem::remove_all(root); std::filesystem::create_directories(root);
	const auto source = root / "source.AppImage";
	{ std::ofstream output(source, std::ios::binary); output << "good"; }
	const auto good_digest = ComputeFileSha256(source);
	auto prepared = PrepareAppImage(source, root / "staged");
	ASSERT_TRUE(prepared.has_value());
	{ std::ofstream output(source, std::ios::binary | std::ios::trunc); output << "evil"; }
	EXPECT_EQ(ComputeFileSha256(prepared->path), good_digest);
	EXPECT_TRUE(PreparedAppImageUnchanged(*prepared));
	std::filesystem::remove_all(root);
}

TEST(PreparedAppImageTest, ReplacingStagedFileIsDetected) {
	const auto root = std::filesystem::temp_directory_path() / "gdownload-prepared-appimage-replace";
	std::filesystem::remove_all(root); std::filesystem::create_directories(root);
	const auto source = root / "source.AppImage";
	{ std::ofstream output(source, std::ios::binary); output << "good"; }
	auto prepared = PrepareAppImage(source, root / "staged");
	ASSERT_TRUE(prepared.has_value());
	std::filesystem::remove(prepared->path);
	{ std::ofstream output(prepared->path, std::ios::binary); output << "evil"; }
	EXPECT_FALSE(PreparedAppImageUnchanged(*prepared));
	std::filesystem::remove_all(root);
}

TEST(PreparedAppImageTest, SameSizeMutationWithRestoredTimestampIsDetected) {
	const auto root = std::filesystem::temp_directory_path() / "gdownload-prepared-appimage-mutation";
	std::filesystem::remove_all(root); std::filesystem::create_directories(root);
	const auto source = root / "source.AppImage";
	{ std::ofstream output(source, std::ios::binary); output << "good"; }
	auto prepared = PrepareAppImage(source, root / "staged");
	ASSERT_TRUE(prepared.has_value());
	{ std::ofstream output(prepared->path, std::ios::binary | std::ios::trunc); output << "evil"; }
	std::filesystem::last_write_time(prepared->path, prepared->modified);
	EXPECT_FALSE(PreparedAppImageUnchanged(*prepared));
	std::filesystem::remove_all(root);
}

TEST(AppImageUpdaterThreadLifecycleTest, VendoredWorkerUsesOwnedThreadAndDestructorStopBarrier) {
	const auto source = std::filesystem::path(GDOWNLOAD_SOURCE_DIR) /
		"lib/AppImageUpdate/src/updater/updater.cpp";
	std::ifstream input(source);
	ASSERT_TRUE(input);
	const std::string text((std::istreambuf_iterator<char>(input)), {});
	EXPECT_NE(text.find("std::unique_ptr<std::thread> thread"), std::string::npos);
	EXPECT_NE(text.find("Updater::~Updater() noexcept"), std::string::npos);
	EXPECT_NE(text.find("thread = std::move(d->thread)"), std::string::npos);
	EXPECT_EQ(text.find("std::thread* thread;"), std::string::npos);
}
}  // namespace
