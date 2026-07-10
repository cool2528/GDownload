#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <ostream>

#include "Module/GDLCore/export.h"
#include "Module/GDLCore/result/result.h"

namespace gdl {
	namespace filesystem {

		enum class AtomicFileReplaceError : std::int64_t {
			kCreateDirectory = 1,
			kOpenTemporaryFile,
			kWrite,
			kFlush,
			kClose,
			kReplace,
			kSyncDirectory,
			kCleanup,
		};

		using AtomicFileWriter = std::function<Result<void>(std::ostream&)>;

		GDLCore_API Result<void> AtomicFileReplace(const std::filesystem::path& target, const AtomicFileWriter& writer);

	}  // namespace filesystem
}  // namespace gdl
