#include "prepared_appimage.h"

#include "update_package_verifier.h"
#include <system_error>
#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace gdl::update {
	namespace {
		std::optional<PreparedAppImage> Inspect(const std::filesystem::path& path) {
			std::error_code ec;
			if (!std::filesystem::is_regular_file(path, ec)) return std::nullopt;
			PreparedAppImage result; result.path = path;
			result.size = std::filesystem::file_size(path, ec); if (ec) return std::nullopt;
			result.modified = std::filesystem::last_write_time(path, ec); if (ec) return std::nullopt;
#ifndef _WIN32
			struct stat status{};
			if (::stat(path.c_str(), &status) != 0) return std::nullopt;
			result.device = static_cast<std::uint64_t>(status.st_dev);
			result.inode = static_cast<std::uint64_t>(status.st_ino);
#endif
			auto digest = ComputeFileSha256(path);
			if (!digest) return std::nullopt;
			result.sha256 = std::move(*digest);
			return result;
		}
	}

	std::optional<PreparedAppImage> PrepareAppImage(const std::filesystem::path& source,
		const std::filesystem::path& staging_root) {
		std::error_code ec;
		std::filesystem::create_directories(staging_root, ec); if (ec) return std::nullopt;
#ifndef _WIN32
		std::filesystem::permissions(staging_root, std::filesystem::perms::owner_all,
			std::filesystem::perm_options::replace, ec); if (ec) return std::nullopt;
#endif
		const auto directory = CreateUniqueUpdateTempPath(staging_root, "");
		if (directory.empty()) return std::nullopt;
		std::filesystem::create_directory(directory, ec); if (ec) return std::nullopt;
#ifndef _WIN32
		std::filesystem::permissions(directory, std::filesystem::perms::owner_all,
			std::filesystem::perm_options::replace, ec); if (ec) return std::nullopt;
#endif
		const auto destination = directory / "update.AppImage";
		if (!std::filesystem::copy_file(source, destination, std::filesystem::copy_options::none, ec) || ec) {
			std::filesystem::remove_all(directory, ec);
			return std::nullopt;
		}
		auto prepared = Inspect(destination);
		if (!prepared) std::filesystem::remove_all(directory, ec);
		return prepared;
	}

	std::optional<PreparedAppImage> CapturePreparedAppImage(const std::filesystem::path& path) {
		return Inspect(path);
	}

	bool PreparedAppImageUnchanged(const PreparedAppImage& prepared) {
		auto current = Inspect(prepared.path);
		return current && current->device == prepared.device && current->inode == prepared.inode &&
			current->size == prepared.size && current->modified == prepared.modified &&
			current->sha256 == prepared.sha256;
	}
}  // namespace gdl::update
