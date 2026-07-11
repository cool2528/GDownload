#include <gtest/gtest.h>

#include <openssl/evp.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "update/update_manifest.h"
#include "update/update_package_verifier.h"
#include "update/platform_package_verifier.h"

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

TEST(UpdateManifestTest, RejectsMalformedExpiredRollbackPlatformHostSchemeSuffixAndDigest) {
	auto key = GenerateKey();
	auto check = [&](nlohmann::json json, ManifestPolicy policy) {
		json["signature"] = Sign(key.key, CanonicalizeManifest(json.dump()).value());
		return VerifyUpdateManifest(json.dump(), PublicKeyBase64(key.key), policy).ok;
	};
	ManifestPolicy policy{"windows-x64", ".exe", {"github.com"}, 41, 1500};
	EXPECT_FALSE(VerifyUpdateManifest("{", PublicKeyBase64(key.key), policy).ok);
	auto json = ManifestJson(); json["expires_at"] = 1400; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); policy.highest_release_id = 42; EXPECT_FALSE(check(json, policy));
	policy.highest_release_id = 41; json["platform"] = "linux-x64"; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); json["asset"]["url"] = "http://github.com/a.exe"; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); json["asset"]["url"] = "https://evil.example/a.exe"; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); json["asset"]["name"] = "a.zip"; EXPECT_FALSE(check(json, policy));
	json = ManifestJson(); json["asset"]["sha256"] = "bad"; EXPECT_FALSE(check(json, policy));
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
}  // namespace
