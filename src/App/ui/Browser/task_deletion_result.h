#pragma once

#include <QVariantMap>
#include "local_content_removal.h"

namespace gdl {
	namespace ui {
		namespace browser {

			struct TaskDeletionResult {
				bool task_removed{false};
				bool aria2_cleaned{false};
				bool history_cleaned{true};
				bool content_requested{false};
				LocalRemovalResult content;
				LocalRemovalResult control_file;

				[[nodiscard]] bool IsCompleteSuccess() const {
					return task_removed && aria2_cleaned && history_cleaned &&
						   (!content_requested ||
							(content.status != LocalRemovalStatus::kFailed &&
							 control_file.status != LocalRemovalStatus::kFailed));
				}

				[[nodiscard]] bool IsPartialSuccess() const {
					return task_removed && !IsCompleteSuccess();
				}

				[[nodiscard]] QVariantMap ToVariantMap() const {
					return {{QStringLiteral("taskRemoved"), task_removed},
							{QStringLiteral("aria2Cleaned"), aria2_cleaned},
							{QStringLiteral("historyCleaned"), history_cleaned},
							{QStringLiteral("contentRequested"), content_requested},
							{QStringLiteral("content"), LocalRemovalResultToVariantMap(content)},
							{QStringLiteral("controlFile"), LocalRemovalResultToVariantMap(control_file)},
							{QStringLiteral("completeSuccess"), IsCompleteSuccess()},
							{QStringLiteral("partialSuccess"), IsPartialSuccess()}};
				}
			};

			struct BulkDeletionResult {
				int total{0};
				int complete{0};
				int partial{0};
				int failed{0};

				void Add(const TaskDeletionResult& result) {
					++total;
					if (result.IsCompleteSuccess()) {
						++complete;
					} else if (result.IsPartialSuccess()) {
						++partial;
					} else {
						++failed;
					}
				}

				[[nodiscard]] QVariantMap ToVariantMap() const {
					return {{QStringLiteral("total"), total},
							{QStringLiteral("complete"), complete},
							{QStringLiteral("partial"), partial},
							{QStringLiteral("failed"), failed}};
				}
			};

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
