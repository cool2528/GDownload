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

// ============================================================================
// ETA 诚实性:剩余时间的分母必须是"进度速率",不能是引擎上报的"线上速率"
//
// 引擎把 speed_bps 的口径改成了"最近 1 秒内线上到达的本文件数据字节"。线上到达的
// 字节完全可能永远落不了盘(未凑齐的块在空闲超时时整段丢弃、part 的 MD4 没过就整段
// 重下、间隙帧与完全重叠帧被直接弃掉),此时 bytes_done 一动不动而 speed 显示几十
// KB/s。拿这个速率去除"进度剩余字节",算出来的 ETA 每秒都差不多、永远不收敛,把
// 一个真卡住的任务粉饰成"还有 28 秒就好"。
// ============================================================================

TEST(DownloadTaskInfoTest, RemainingTimeIsUnknownWhenWireSpeedFlowsButProgressNeverMoved) {
	DownloadTaskInfo task;
	task.set_task_state(TaskState::kActive);
	task.set_task_total_size(100 * 1024 * 1024);
	task.set_task_current_size(10 * 1024 * 1024);
	// 线上确实有 96 KiB/s 在流,但没有任何一次采样让 current_size 前进过
	task.set_task_download_speed(96 * 1024);

	EXPECT_EQ(task.remaining_time(), -1);
	EXPECT_EQ(task.FormatRemainingTime(), QStringLiteral("Unknown"));
}

TEST(DownloadTaskModelTest, ExposesRawByteSizeAndProgressStalledRoles) {
	DownloadTaskModel model;
	const auto role_names = model.roleNames().values();

	// QML 的可见性判据必须能拿到未经格式化的原始数值:FormatFileSize 只保留两位小数,
	// 10 GB 量级上 5 MB 的缺口会被舍成同一个字符串
	EXPECT_TRUE(role_names.contains(QByteArray("totalSizeBytes")));
	EXPECT_TRUE(role_names.contains(QByteArray("currentSizeBytes")));
	// "在收但落不了盘"必须以一个独立信号送到界面,不能只留在引擎里
	EXPECT_TRUE(role_names.contains(QByteArray("progressStalled")));
}

TEST(DownloadTaskModelTest, RawByteSizeRolesReturnUnformattedNumbers) {
	DownloadTaskModel model;
	DownloadTaskInfo task;
	task.set_task_id(QStringLiteral("bytes-gid"));
	task.set_task_state(TaskState::kComplete);
	task.set_task_total_size(10737418240LL);   // 10.00 GB
	task.set_task_current_size(10732175360LL); // 同样格式化成 "10.00 GB",实际少 5 MB
	model.AddTask(task);

	const auto roles		 = model.roleNames();
	const int total_role	 = roles.key(QByteArray("totalSizeBytes"), -1);
	const int current_role	 = roles.key(QByteArray("currentSizeBytes"), -1);
	ASSERT_NE(total_role, -1);
	ASSERT_NE(current_role, -1);

	const QModelIndex row = model.index(0, 0);
	EXPECT_EQ(model.data(row, total_role).toLongLong(), 10737418240LL);
	EXPECT_EQ(model.data(row, current_role).toLongLong(), 10732175360LL);
	// 格式化后的两条 role 确实相同 —— 这正是不能拿它们比大小的原因
	EXPECT_EQ(model.data(row, DownloadTaskModel::kTaskTotalSize).toString(),
			  model.data(row, DownloadTaskModel::kTaskCurrentSize).toString());
}

namespace {
constexpr std::int64_t kAichBlock	 = 184320;			  // eD2k 一个 AICH 块
constexpr std::int64_t kFileSize	 = 700 * 1024 * 1024;  // 700 MiB
constexpr std::int64_t kWireSpeed	 = 96 * 1024;		  // 线上 96 KiB/s

DownloadTaskInfo MakeEd2kActiveTask() {
	DownloadTaskInfo task;
	task.set_task_id(QStringLiteral("ed2k-3d366ed505b977fc61c9a6ee01e96329"));
	task.set_task_state(TaskState::kActive);
	task.set_task_file_name(QStringLiteral("Half-dead.rar"));
	task.set_task_total_size(kFileSize);
	task.set_task_current_size(12 * 1024 * 1024);
	task.set_task_download_speed(kWireSpeed);
	return task;
}

// 模拟桌面端每秒一次的采样:引擎 payload 每次都重建一份全新的 TaskInfo,进度速率的
// 历史窗口由模型从旧条目接过来(与 BrowserManagerImpl 的 sigUpdateTasksMessage 路径同构)。
DownloadTaskInfo SampleAt(DownloadTaskModel& model, const DownloadTaskInfo& base,
						  std::int64_t current_size, std::int64_t wire_speed, std::int64_t now_ms) {
	DownloadTaskInfo fresh = base;
	fresh.set_task_current_size(current_size);
	fresh.set_task_download_speed(wire_speed);
	model.UpdateTaskById(fresh.task_id(), fresh, now_ms);
	return fresh;
}
}  // namespace

TEST(DownloadTaskModelTest, EtaStaysUnknownWhileWireDataArrivesButNothingLands) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	// 两分钟采样:线上一直有 96 KiB/s 在流,bytes_done 一个字节都没前进
	for (std::int64_t t = 1000; t <= 120000; t += 1000) {
		SampleAt(model, base, base.task_current_size(), kWireSpeed, t);
	}

	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	EXPECT_EQ(live->progress_rate_bps(), 0);
	EXPECT_EQ(live->remaining_time(), -1);
	EXPECT_TRUE(live->progress_stalled());
}

TEST(DownloadTaskModelTest, EtaIsComputedFromProgressRateNotWireRate) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	// 每 10 秒落一个 AICH 块 => 进度速率 ≈ 18432 B/s;线上速率报成它的 10 倍
	std::int64_t current = base.task_current_size();
	for (std::int64_t t = 1000; t <= 120000; t += 1000) {
		if (t % 10000 == 0) current += kAichBlock;
		SampleAt(model, base, current, kAichBlock, t);
	}

	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	EXPECT_FALSE(live->progress_stalled());

	const std::int64_t expected_rate = kAichBlock / 10;
	EXPECT_NEAR(static_cast<double>(live->progress_rate_bps()), static_cast<double>(expected_rate),
				expected_rate * 0.25);

	const std::int64_t remaining = kFileSize - current;
	const int expected_eta		 = static_cast<int>(remaining / expected_rate);
	EXPECT_NEAR(live->remaining_time(), expected_eta, expected_eta * 0.25);
}

// 非回归:aria2/BT 任务的 speed 与 progress 同源,改动不能让那一侧的 ETA 退化成 Unknown
TEST(DownloadTaskModelTest, SameSourceSpeedAndProgressKeepsEtaMatchingSpeed) {
	DownloadTaskModel model;
	DownloadTaskInfo base;
	base.set_task_id(QStringLiteral("aria2-gid"));
	base.set_task_state(TaskState::kActive);
	base.set_task_total_size(1024LL * 1024 * 1024);
	base.set_task_current_size(0);
	model.AddTask(base, 0);

	constexpr std::int64_t kPerSecond = 1024 * 1024;  // 每秒 1 MiB,进度同步推进
	std::int64_t current			  = 0;
	for (std::int64_t t = 1000; t <= 30000; t += 1000) {
		current += kPerSecond;
		SampleAt(model, base, current, kPerSecond, t);
	}

	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	EXPECT_FALSE(live->progress_stalled());
	const int expected_eta = static_cast<int>((base.task_total_size() - current) / kPerSecond);
	EXPECT_NEAR(live->remaining_time(), expected_eta, expected_eta * 0.1);
}

// 停滞告警的阈值必须显著高于"最慢单源凑够一个块"的耗时(5 KiB/s ⇒ 37 秒),
// 否则正常的慢速下载会被天天误报成故障,告警随即被用户当噪音无视
TEST(DownloadTaskModelTest, StallIsNotReportedWithinOneSlowBlockInterval) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	for (std::int64_t t = 1000; t <= 40000; t += 1000) {
		SampleAt(model, base, base.task_current_size(), kWireSpeed, t);
	}
	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	EXPECT_FALSE(live->progress_stalled());
}

// 线上一个字节都没来 = "没有源",与"在收但存不下"是两种完全不同的故障,不能混用同一句告警
TEST(DownloadTaskModelTest, StallIsNotReportedWhenNothingArrivesOnTheWire) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	for (std::int64_t t = 1000; t <= 180000; t += 1000) {
		SampleAt(model, base, base.task_current_size(), 0, t);
	}
	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	EXPECT_FALSE(live->progress_stalled());
	EXPECT_EQ(live->remaining_time(), -1);
}

// 引擎的三条"白流"路径里,part 的 MD4 没过、整个 part 重下这一条最隐蔽:bytes_done
// 一路涨到接近 part 末尾又被打回原点,如此循环。每次回退都把"停滞多久了"清零的话,
// 这个任务可以整夜都在收数据、整夜都在原地踏步,而界面上永远不会报警。
// 判据必须以"历史最高水位"为准:只有真正超过以往的最高进度才算前进过。
TEST(DownloadTaskModelTest, RepeatedPartResetsAreReportedAsStalled) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	// 每秒进 50 KiB,每 30 秒整个 part 校验失败被打回起点 —— 净进度恒为 0
	std::int64_t current = base.task_current_size();
	for (std::int64_t t = 1000; t <= 150000; t += 1000) {
		if (t % 30000 == 0) {
			current = base.task_current_size();
		}
		else {
			current += 50 * 1024;
		}
		SampleAt(model, base, current, kWireSpeed, t);
	}

	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	EXPECT_EQ(live->progress_rate_bps(), 0);
	EXPECT_EQ(live->remaining_time(), -1);
	EXPECT_TRUE(live->progress_stalled());
}

// part 的 MD4 没过时引擎会 reset_part(),bytes_done 回退。此时速率必须变成"未知",
// 绝不能拿回退前后的差值算出负速率或荒谬的剩余时间
TEST(DownloadTaskModelTest, ProgressRollbackMakesTheRateUnknown) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	std::int64_t current = base.task_current_size();
	for (std::int64_t t = 1000; t <= 20000; t += 1000) {
		current += kAichBlock;
		SampleAt(model, base, current, kWireSpeed, t);
	}
	// 整个 part(9.28 MB)校验失败被重下
	SampleAt(model, base, base.task_current_size(), kWireSpeed, 21000);

	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	EXPECT_GE(live->progress_rate_bps(), 0);
	EXPECT_EQ(live->remaining_time(), -1);  // 窗口清空 => 速率未知 => ETA 未知
}

TEST(DownloadTaskModelTest, StallFlagIsExposedThroughTheProgressStalledRole) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);
	for (std::int64_t t = 1000; t <= 120000; t += 1000) {
		SampleAt(model, base, base.task_current_size(), kWireSpeed, t);
	}

	const auto roles	   = model.roleNames();
	const int stalled_role = roles.key(QByteArray("progressStalled"), -1);
	ASSERT_NE(stalled_role, -1);
	EXPECT_TRUE(model.data(model.index(0, 0), stalled_role).toBool());
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

TEST(DownloadTaskModelTest, ReTombstoneAfterClearSurvivesFifoEviction) {
	DownloadTaskModel model;
	DownloadTaskInfo task;
	task.set_task_id(QStringLiteral("recycled-gid"));
	task.set_task_state(TaskState::kComplete);
	model.AddTask(task);
	ASSERT_TRUE(model.RemoveTaskById(task.task_id()));
	model.ClearTombstone(task.task_id());
	model.AddTask(task);
	ASSERT_TRUE(model.RemoveTaskById(task.task_id()));
	ASSERT_TRUE(model.IsTombstoned(task.task_id()));

	// 填充 kMaxTombstones-1(=511)个其他墓碑:若 ClearTombstone 在 FIFO 队列中
	// 留下陈旧条目,重建的墓碑会在淘汰弹出旧条目时被从哈希表误删
	for (int i = 0; i < 511; ++i) {
		DownloadTaskInfo other;
		other.set_task_id(QStringLiteral("other-%1").arg(i));
		other.set_task_state(TaskState::kComplete);
		model.AddTask(other);
		ASSERT_TRUE(model.RemoveTaskById(other.task_id()));
	}

	EXPECT_TRUE(model.IsTombstoned(QStringLiteral("recycled-gid")));
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
