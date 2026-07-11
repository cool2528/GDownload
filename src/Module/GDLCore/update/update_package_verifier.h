#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "export.h"

namespace gdl::update {
	struct PackageVerificationResult { bool ok{false}; std::string error; };
	GDLCore_API std::optional<std::string> ParseGithubSha256Digest(const std::string& digest);
	GDLCore_API std::optional<std::string> ComputeFileSha256(const std::filesystem::path& path);
	GDLCore_API PackageVerificationResult VerifyUpdatePackage(const std::filesystem::path& path,
		std::int64_t expected_size, const std::string& expected_sha256);
	GDLCore_API std::filesystem::path CreateUniqueUpdateTempPath(const std::filesystem::path& directory,
		const std::string& suffix);
}  // namespace gdl::update
