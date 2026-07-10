#pragma once

#ifdef _WIN32
#include <Windows.h>

namespace gdl {
	namespace filesystem {
		namespace detail {

			enum class WindowsReplaceFailureAction {
				kFailAndCleanup,
				kMoveToMissingTarget,
				kOldTargetIntact,
				kRestoreBackup,
			};

			constexpr WindowsReplaceFailureAction ClassifyWindowsReplaceError(DWORD error) {
				if (error == ERROR_FILE_NOT_FOUND) return WindowsReplaceFailureAction::kMoveToMissingTarget;
				if (error == ERROR_UNABLE_TO_MOVE_REPLACEMENT) return WindowsReplaceFailureAction::kOldTargetIntact;
				if (error == ERROR_UNABLE_TO_MOVE_REPLACEMENT_2) return WindowsReplaceFailureAction::kRestoreBackup;
				return WindowsReplaceFailureAction::kFailAndCleanup;
			}

		}  // namespace detail
	}  // namespace filesystem
}  // namespace gdl
#endif
