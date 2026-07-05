#pragma once

#include <QObject>
#include <QString>

namespace gdl {
	namespace ui {
		namespace browser {

			struct StoppedTaskDeletionDecision {
				bool remove_local_task{true};
				bool show_cleanup_warning{false};
				QString warning_message;
			};

			inline StoppedTaskDeletionDecision DecideStoppedTaskDeletionAfterAria2Cleanup(
				bool cleanup_succeeded, const QString& cleanup_error) {
				if (cleanup_succeeded) {
					return {};
				}

				const QString detail = cleanup_error.trimmed();
				return {.remove_local_task = true,
						.show_cleanup_warning = true,
						.warning_message =
							detail.isEmpty()
								? QObject::tr("Task was removed locally, but aria2 cleanup failed.")
								: QObject::tr("Task was removed locally, but aria2 cleanup failed: %1").arg(detail)};
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
