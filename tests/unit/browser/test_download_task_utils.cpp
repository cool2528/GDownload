#include <gtest/gtest.h>

#include <utility>

#include "Browser/download_task_utils.h"

namespace {

using gdl::ui::browser::BuildRetryTaskRequest;
using gdl::ui::browser::DownloadRecordFromTaskInfo;
using gdl::ui::browser::DownloadTaskInfo;
using gdl::ui::browser::DownloadTaskInfoFromAria2Object;
using gdl::ui::browser::DownloadTaskInfoFromRecord;
using gdl::ui::browser::DownloadTaskModel;
using gdl::ui::browser::ShouldRestoreHistoryTask;
using gdl::ui::browser::TaskState;

DownloadTaskInfo MakeFailedTask() {
	DownloadTaskInfo task;
	task.set_task_id(QStringLiteral("failed-gid"));
	task.set_task_state(TaskState::kError);
	task.set_task_file_name(QStringLiteral("archive.zip"));
	task.set_task_save_path(QStringLiteral("D:/Downloads/archive.zip"));
	task.set_task_download_link(QStringLiteral("https://example.com/archive.zip"));
	task.set_task_error_code(QStringLiteral("3"));
	task.set_task_error_message(QStringLiteral("Resource not found"));
	return task;
}

TEST(DownloadTaskInfoTest, PreservesErrorDetailsAcrossCopiesAndMoves) {
	const DownloadTaskInfo source = MakeFailedTask();

	DownloadTaskInfo copy_constructed(source);
	EXPECT_EQ(copy_constructed.task_error_code(), QStringLiteral("3"));
	EXPECT_EQ(copy_constructed.task_error_message(), QStringLiteral("Resource not found"));

	DownloadTaskInfo copy_assigned;
	copy_assigned = source;
	EXPECT_EQ(copy_assigned.task_error_code(), QStringLiteral("3"));
	EXPECT_EQ(copy_assigned.task_error_message(), QStringLiteral("Resource not found"));

	DownloadTaskInfo move_constructed(std::move(copy_constructed));
	EXPECT_EQ(move_constructed.task_error_code(), QStringLiteral("3"));
	EXPECT_EQ(move_constructed.task_error_message(), QStringLiteral("Resource not found"));

	DownloadTaskInfo move_assigned;
	move_assigned = std::move(copy_assigned);
	EXPECT_EQ(move_assigned.task_error_code(), QStringLiteral("3"));
	EXPECT_EQ(move_assigned.task_error_message(), QStringLiteral("Resource not found"));
}

TEST(DownloadTaskModelTest, ExposesErrorCodeAndMessageRoles) {
	DownloadTaskModel model;
	model.AddTask(MakeFailedTask());

	const auto roles = model.roleNames();
	EXPECT_EQ(roles.value(DownloadTaskModel::kTaskErrorCode), QByteArray("errorCode"));
	EXPECT_EQ(roles.value(DownloadTaskModel::kTaskErrorMessage), QByteArray("errorMessage"));

	const QModelIndex row = model.index(0, 0);
	EXPECT_EQ(model.data(row, DownloadTaskModel::kTaskErrorCode).toString(), QStringLiteral("3"));
	EXPECT_EQ(model.data(row, DownloadTaskModel::kTaskErrorMessage).toString(),
			  QStringLiteral("Resource not found"));
}

TEST(DownloadTaskModelTest, CanClearStoppedTaskTombstoneForNewLifecycle) {
	DownloadTaskModel model;
	DownloadTaskInfo task;
	task.set_task_id(QStringLiteral("reused-gid"));
	task.set_task_state(TaskState::kComplete);
	model.AddTask(task);

	ASSERT_TRUE(model.RemoveTaskById(task.task_id()));
	ASSERT_TRUE(model.IsTombstoned(task.task_id()));

	model.ClearTombstone(task.task_id());
	EXPECT_FALSE(model.IsTombstoned(task.task_id()));
}

TEST(DownloadTaskUtilsTest, ParsesAria2ErrorDetails) {
	const nlohmann::json object = {
		{"gid", "failed-gid"},
		{"status", "error"},
		{"totalLength", "1024"},
		{"completedLength", "128"},
		{"downloadSpeed", "0"},
		{"connections", "0"},
		{"errorCode", "3"},
		{"errorMessage", "Resource not found"},
		{"dir", "D:/Downloads"},
		{"files", nlohmann::json::array({{{"path", "D:/Downloads/archive.zip"},
											 {"uris", nlohmann::json::array({{{"uri", "https://example.com/archive.zip"}}})}}})},
	};

	const DownloadTaskInfo task = DownloadTaskInfoFromAria2Object(object);

	EXPECT_EQ(task.task_state(), TaskState::kError);
	EXPECT_EQ(task.task_error_code(), QStringLiteral("3"));
	EXPECT_EQ(task.task_error_message(), QStringLiteral("Resource not found"));
}

TEST(DownloadTaskUtilsTest, RoundTripsHistoryErrorMessage) {
	const DownloadTaskInfo source = MakeFailedTask();

	const auto record = DownloadRecordFromTaskInfo(source);
	EXPECT_EQ(record.error_message, "Resource not found");

	const DownloadTaskInfo restored = DownloadTaskInfoFromRecord(record);
	EXPECT_EQ(restored.task_error_message(), QStringLiteral("Resource not found"));
	EXPECT_EQ(restored.task_state(), TaskState::kError);
}

TEST(DownloadTaskUtilsTest, BuildsRetryRequestFromOriginalUrlAndPath) {
	const auto request = BuildRetryTaskRequest(MakeFailedTask());

	ASSERT_TRUE(request.has_value());
	ASSERT_EQ(request->urls.size(), 1);
	EXPECT_EQ(request->urls.front().toString(), QStringLiteral("https://example.com/archive.zip"));
	EXPECT_EQ(request->options.value(QStringLiteral("dir")).toString(), QStringLiteral("D:/Downloads"));
	EXPECT_EQ(request->options.value(QStringLiteral("out")).toString(), QStringLiteral("archive.zip"));
}

TEST(DownloadTaskUtilsTest, RejectsRetryForNonErrorOrMissingDownloadLink) {
	DownloadTaskInfo completed = MakeFailedTask();
	completed.set_task_state(TaskState::kComplete);
	EXPECT_FALSE(BuildRetryTaskRequest(completed).has_value());

	DownloadTaskInfo missing_link;
	missing_link.set_task_state(TaskState::kError);
	EXPECT_FALSE(BuildRetryTaskRequest(missing_link).has_value());
}

TEST(DownloadTaskUtilsTest, RestoresRetryableFailureWhenTargetFileDoesNotExist) {
	const DownloadTaskInfo failed = MakeFailedTask();
	EXPECT_TRUE(ShouldRestoreHistoryTask(failed, false));

	DownloadTaskInfo completed = failed;
	completed.set_task_state(TaskState::kComplete);
	EXPECT_FALSE(ShouldRestoreHistoryTask(completed, false));
	EXPECT_TRUE(ShouldRestoreHistoryTask(completed, true));

	DownloadTaskInfo failed_without_link;
	failed_without_link.set_task_state(TaskState::kError);
	failed_without_link.set_task_error_message(QStringLiteral("Metadata error"));
	EXPECT_TRUE(ShouldRestoreHistoryTask(failed_without_link, false));
}

}  // namespace
