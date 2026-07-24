#pragma once

#include <string>

#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QString>

namespace gdl {
	namespace ui {
		namespace browser {

			enum class StoppedTaskAria2CleanupStatus {
				kSucceeded,
				kAlreadyMissing,
				kFailed,
			};

			inline QString ExtractAria2RpcErrorMessage(const std::string& body) {
				const QJsonDocument document =
					QJsonDocument::fromJson(QByteArray::fromStdString(body));
				if (!document.isObject()) return {};
				const QJsonObject error = document.object().value(QStringLiteral("error")).toObject();
				return error.value(QStringLiteral("message")).toString().trimmed();
			}

			inline bool IsMissingAria2ResultError(const QString& message) {
				const QString lower = message.toLower();
				return lower.contains(QStringLiteral("gid")) &&
					(lower.contains(QStringLiteral("not found")) ||
					 lower.contains(QStringLiteral("does not exist")));
			}

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

				const QString detail = cleanup_error.trimmed();
				return {.remove_local_task = true,
						.aria2_cleaned = false,
						.show_cleanup_warning = true,
						.warning_message =
							detail.isEmpty()
								? QObject::tr("Task was removed locally, but aria2 cleanup failed.")
								: QObject::tr("Task was removed locally, but aria2 cleanup failed: %1").arg(detail)};
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
