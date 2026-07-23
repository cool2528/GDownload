#include "platform_package_verifier.h"

#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <Windows.h>
#include <Softpub.h>
#include <Wincrypt.h>
#include <Wintrust.h>
#include <iomanip>
#include <sstream>
#endif

namespace gdl::update {
	bool SignerPinMatches(const std::string& expected_pin, const std::string& actual_spki_sha256) {
		constexpr std::string_view prefix = "sha256:";
		if (!expected_pin.starts_with(prefix) || expected_pin.size() != prefix.size() + 64 ||
			actual_spki_sha256.size() != 64) return false;
		std::string expected = expected_pin.substr(prefix.size());
		std::string actual = actual_spki_sha256;
		std::transform(expected.begin(), expected.end(), expected.begin(), [](unsigned char c) { return std::tolower(c); });
		std::transform(actual.begin(), actual.end(), actual.begin(), [](unsigned char c) { return std::tolower(c); });
		return expected == actual && std::all_of(expected.begin(), expected.end(), [](unsigned char c) { return std::isxdigit(c); });
	}

#ifdef _WIN32
	namespace {
		std::optional<std::string> SignerSpkiSha256(const std::filesystem::path& path) {
			HCERTSTORE store = nullptr; HCRYPTMSG message = nullptr;
			DWORD encoding = 0, content = 0, format = 0;
			if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE, path.c_str(), CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
				CERT_QUERY_FORMAT_FLAG_BINARY, 0, &encoding, &content, &format, &store, &message, nullptr)) return std::nullopt;
			DWORD size = 0;
			if (!CryptMsgGetParam(message, CMSG_SIGNER_INFO_PARAM, 0, nullptr, &size)) { CertCloseStore(store, 0); CryptMsgClose(message); return std::nullopt; }
			std::vector<unsigned char> buffer(size);
			if (!CryptMsgGetParam(message, CMSG_SIGNER_INFO_PARAM, 0, buffer.data(), &size)) { CertCloseStore(store, 0); CryptMsgClose(message); return std::nullopt; }
			auto* signer = reinterpret_cast<CMSG_SIGNER_INFO*>(buffer.data());
			CERT_INFO info{}; info.Issuer = signer->Issuer; info.SerialNumber = signer->SerialNumber;
			PCCERT_CONTEXT certificate = CertFindCertificateInStore(store, encoding, 0, CERT_FIND_SUBJECT_CERT, &info, nullptr);
			std::optional<std::string> result;
			if (certificate) {
				BYTE digest[32]; DWORD digest_size = sizeof(digest);
				if (CryptHashPublicKeyInfo(0, CALG_SHA_256, 0, certificate->dwCertEncodingType,
					&certificate->pCertInfo->SubjectPublicKeyInfo, digest, &digest_size)) {
					std::ostringstream out; out << std::hex << std::setfill('0');
					for (DWORD i = 0; i < digest_size; ++i) out << std::setw(2) << static_cast<int>(digest[i]);
					result = out.str();
				}
				CertFreeCertificateContext(certificate);
			}
			CertCloseStore(store, 0); CryptMsgClose(message); return result;
		}

		class WinAuthenticodeVerifier final : public IPlatformPackageVerifier {
		   public:
			PackageVerificationResult Verify(const std::filesystem::path& package,
				const std::string& expected_signer_pin) const override {
				// 空 pin 表示未做代码签名(本项目为开源自签方案,无付费 Authenticode 证书),
				// 跳过 Authenticode 签名者校验这一层；更新包的完整性仍由上游 ed25519 签名的更新清单
				// (校验版本号/下载地址/SHA-256)以及安装前的 SHA-256 完整性校验共同保证。
				// 若配置了非空 pin(拥有证书),则继续走下方完整的信任链验证 + SPKI 指纹校验,严格 fail-closed。
				if (expected_signer_pin.empty()) return {true, {}};
				WINTRUST_FILE_INFO file{}; file.cbStruct = sizeof(file); file.pcwszFilePath = package.c_str();
				WINTRUST_DATA data{}; data.cbStruct = sizeof(data); data.dwUIChoice = WTD_UI_NONE;
				data.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN; data.dwUnionChoice = WTD_CHOICE_FILE;
				data.pFile = &file; data.dwStateAction = WTD_STATEACTION_VERIFY;
				GUID policy = WINTRUST_ACTION_GENERIC_VERIFY_V2;
				const LONG status = WinVerifyTrust(nullptr, &policy, &data);
				data.dwStateAction = WTD_STATEACTION_CLOSE; WinVerifyTrust(nullptr, &policy, &data);
				if (status != ERROR_SUCCESS) return {false, "Authenticode verification failed"};
				auto pin = SignerSpkiSha256(package);
				if (!pin || !SignerPinMatches(expected_signer_pin, *pin)) return {false, "update signer pin mismatch"};
				return {true, {}};
			}
		};
	}
#endif

	std::unique_ptr<IPlatformPackageVerifier> CreatePlatformPackageVerifier() {
#ifdef _WIN32
		return std::make_unique<WinAuthenticodeVerifier>();
#else
		return {};
#endif
	}
}  // namespace gdl::update
