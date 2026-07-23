#include "update_package_verifier.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>

namespace gdl::update {
	std::optional<std::string> ParseGithubSha256Digest(const std::string& digest) {
		constexpr std::string_view prefix = "sha256:";
		if (!digest.starts_with(prefix)) return std::nullopt;
		std::string value = digest.substr(prefix.size());
		if (value.size() != 64 || !std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isxdigit(c); }))
			return std::nullopt;
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
		return value;
	}

	std::optional<std::string> ComputeFileSha256(const std::filesystem::path& path) {
		std::ifstream input(path, std::ios::binary);
		if (!input) return std::nullopt;
		EVP_MD_CTX* context = EVP_MD_CTX_new();
		if (!context || EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1) { EVP_MD_CTX_free(context); return std::nullopt; }
		std::array<char, 64 * 1024> buffer{};
		while (input) {
			input.read(buffer.data(), buffer.size());
			const auto count = input.gcount();
			if (count > 0 && EVP_DigestUpdate(context, buffer.data(), static_cast<size_t>(count)) != 1) {
				EVP_MD_CTX_free(context); return std::nullopt;
			}
		}
		unsigned char digest[EVP_MAX_MD_SIZE]; unsigned int size = 0;
		if (!input.eof() || EVP_DigestFinal_ex(context, digest, &size) != 1) { EVP_MD_CTX_free(context); return std::nullopt; }
		EVP_MD_CTX_free(context);
		std::ostringstream out; out << std::hex << std::setfill('0');
		for (unsigned int i = 0; i < size; ++i) out << std::setw(2) << static_cast<int>(digest[i]);
		return out.str();
	}

	PackageVerificationResult VerifyUpdatePackage(const std::filesystem::path& path,
		std::int64_t expected_size, const std::string& expected_sha256) {
		std::error_code ec;
		if (!std::filesystem::is_regular_file(path, ec)) return {false, "update package is missing"};
		if (static_cast<std::int64_t>(std::filesystem::file_size(path, ec)) != expected_size || ec)
			return {false, "update package size mismatch"};
		auto actual = ComputeFileSha256(path);
		std::string expected = expected_sha256;
		std::transform(expected.begin(), expected.end(), expected.begin(), [](unsigned char c) { return std::tolower(c); });
		if (!actual || *actual != expected) return {false, "update package SHA-256 mismatch"};
		return {true, {}};
	}

	std::filesystem::path CreateUniqueUpdateTempPath(const std::filesystem::path& directory,
		const std::string& suffix) {
		std::array<unsigned char, 16> random{};
		if (RAND_bytes(random.data(), random.size()) != 1) return {};
		std::ostringstream name; name << "gdownload-update-" << std::hex << std::setfill('0');
		for (auto byte : random) name << std::setw(2) << static_cast<int>(byte);
		name << suffix;
		return directory / name.str();
	}
}  // namespace gdl::update
