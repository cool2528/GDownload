#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "export.h"

namespace gdl::update {
	struct PreparedAppImage {
		std::filesystem::path path;
		std::uint64_t device{0};
		std::uint64_t inode{0};
		std::uintmax_t size{0};
		std::filesystem::file_time_type modified;
		std::string sha256;
	};
	GDLCore_API std::optional<PreparedAppImage> PrepareAppImage(const std::filesystem::path& source,
		const std::filesystem::path& staging_root);
	GDLCore_API std::optional<PreparedAppImage> CapturePreparedAppImage(const std::filesystem::path& path);
	GDLCore_API bool PreparedAppImageUnchanged(const PreparedAppImage& prepared);
}  // namespace gdl::update
