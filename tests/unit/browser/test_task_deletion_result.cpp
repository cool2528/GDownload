#include <gtest/gtest.h>

#include "Browser/task_deletion_result.h"
#include "Browser/local_content_removal.h"

namespace {

using gdl::ui::browser::BulkDeletionResult;
using gdl::ui::browser::TaskDeletionResult;

TEST(TaskDeletionResultTest, CompleteSuccessIncludesRecordOnlyDeletion) {
	TaskDeletionResult complete{.task_removed = true, .aria2_cleaned = true};
	TaskDeletionResult record_only{.task_removed = true,
								  .aria2_cleaned = true,
								  .content_requested = false};

	EXPECT_TRUE(complete.IsCompleteSuccess());
	EXPECT_TRUE(record_only.IsCompleteSuccess());
	EXPECT_FALSE(record_only.IsPartialSuccess());
}

TEST(TaskDeletionResultTest, ContentOrControlFailureIsPartialAfterTaskRemoval) {
	TaskDeletionResult content_failure{.task_removed = true,
									 .aria2_cleaned = true,
									 .content_requested = true,
									 .content = {.status = gdl::ui::browser::LocalRemovalStatus::kFailed}};
	TaskDeletionResult control_failure{.task_removed = true,
									 .aria2_cleaned = true,
									 .content_requested = true,
									 .control_file = {.status = gdl::ui::browser::LocalRemovalStatus::kFailed}};

	EXPECT_TRUE(content_failure.IsPartialSuccess());
	EXPECT_TRUE(control_failure.IsPartialSuccess());
	EXPECT_FALSE(content_failure.IsCompleteSuccess());
	EXPECT_FALSE(control_failure.IsCompleteSuccess());
}

TEST(TaskDeletionResultTest, DirectoryContentRemovalCanBeCompleteSuccess) {
	TaskDeletionResult result{.task_removed = true,
						  .aria2_cleaned = true,
						  .content_requested = true,
						  .content = {.status = gdl::ui::browser::LocalRemovalStatus::kRemoved,
									  .path = QStringLiteral("download-directory")},
						  .control_file = {.status = gdl::ui::browser::LocalRemovalStatus::kNotFound}};

	EXPECT_TRUE(result.IsCompleteSuccess());
	const QVariantMap map = result.ToVariantMap();
	EXPECT_EQ(map.value(QStringLiteral("content")).toMap().value(QStringLiteral("status")).toString(),
			  QStringLiteral("removed"));
}

TEST(TaskDeletionResultTest, VariantMapExposesStableDeletionContract) {
	const QVariantMap map = TaskDeletionResult{}.ToVariantMap();
	const QSet<QString> expected{QStringLiteral("taskRemoved"), QStringLiteral("aria2Cleaned"),
		QStringLiteral("historyCleaned"), QStringLiteral("contentRequested"),
		QStringLiteral("content"), QStringLiteral("controlFile"),
		QStringLiteral("completeSuccess"), QStringLiteral("partialSuccess")};
	EXPECT_EQ(QSet<QString>(map.keyBegin(), map.keyEnd()), expected);
	const QVariantMap content = map.value(QStringLiteral("content")).toMap();
	EXPECT_TRUE(content.contains(QStringLiteral("status")));
	EXPECT_TRUE(content.contains(QStringLiteral("path")));
	EXPECT_TRUE(content.contains(QStringLiteral("error")));
	EXPECT_TRUE(content.contains(QStringLiteral("errorCode")));
	EXPECT_TRUE(content.contains(QStringLiteral("removedCount")));
	EXPECT_TRUE(content.contains(QStringLiteral("partialPossible")));
}

TEST(TaskDeletionResultTest, RpcFailureIsFailed) {
	TaskDeletionResult rpc_failure{.task_removed = false, .aria2_cleaned = false};

	EXPECT_FALSE(rpc_failure.IsCompleteSuccess());
	EXPECT_FALSE(rpc_failure.IsPartialSuccess());
}

TEST(BulkDeletionResultTest, CountsMixedOutcomes) {
	BulkDeletionResult bulk;
	bulk.Add({.task_removed = true, .aria2_cleaned = true});
	bulk.Add({.task_removed = true,
			  .aria2_cleaned = true,
			  .content_requested = true,
			  .content = {.status = gdl::ui::browser::LocalRemovalStatus::kFailed}});
	bulk.Add({.task_removed = false, .aria2_cleaned = false});

	EXPECT_EQ(bulk.total, 3);
	EXPECT_EQ(bulk.complete, 1);
	EXPECT_EQ(bulk.partial, 1);
	EXPECT_EQ(bulk.failed, 1);
}

}  // namespace
