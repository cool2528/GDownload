#pragma once

#include <QString>

namespace gdl {
	namespace ui {
		namespace browser {

			enum class StoppedTaskAria2CleanupStatus {
				kSucceeded,
				kAlreadyMissing,
				kFailed,
			};

			struct StoppedTaskDeletionDecision {
				bool remove_local_task{false};
				bool aria2_cleaned{false};
				bool show_cleanup_warning{false};
				QString warning_message;
			};

			inline StoppedTaskDeletionDecision DecideStoppedTaskDeletionAfterAria2Cleanup(
				StoppedTaskAria2CleanupStatus status, const QString& cleanup_error) {
				if (status == StoppedTaskAria2CleanupStatus::kSucceeded ||
					status == StoppedTaskAria2CleanupStatus::kAlreadyMissing) {
					return {.remove_local_task = true, .aria2_cleaned = true};
				}

				Q_UNUSED(cleanup_error);
				return {.remove_local_task = false,
						.aria2_cleaned = false,
						.show_cleanup_warning = false,
						.warning_message = {}};
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
