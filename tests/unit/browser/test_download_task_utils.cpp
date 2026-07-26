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
using gdl::ui::browser::StallKind;
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
// 引擎真实的进度推进粒度。桌面端曾经按"一个 AICH 块单独落盘"来推阈值,那是错的:
//   download.cpp:37    inline constexpr std::size_t kPipelineDepth = 3;
//   download.cpp:1683  auto batch = st.alloc.next_blocks_for_parts(servable, kPipelineDepth);
//   c2c_connection.cpp:76  accumulate_blocks 的主循环是 while (!all_done()),要求这一批
//                          3 个区间全部凑齐才返回(健康的慢速流既不触发 OUTOFPARTREQS
//                          也不触发 100 秒空闲超时,不会提前 break);
//   download.cpp:1863  sync_progress() 只在 write_block_async 成功之后调用。
// 于是 bytes_done 的最小推进单位是一整批 3 × 184320 = 552960 字节。任何按单块粒度
// 臆造出来的输入都不是引擎会上报的形状,拿它写出来的护栏测的是不可能发生的事。
constexpr std::int64_t kEngineProgressStep = 3 * kAichBlock;
constexpr std::int64_t kFileSize	 = 700 * 1024 * 1024;  // 700 MiB
constexpr std::int64_t kWireSpeed	 = 96 * 1024;		  // 线上 96 KiB/s
// 引擎的 PART_SIZE。alloc.reset_part() 会把 bytes_done 整整回退这么多 —— 这是真实
// 的回退量,任何用"每 30 秒退 1.5 MB"之类臆造输入写出来的护栏都不算数。
constexpr std::int64_t kEd2kPartSize = 9728000;

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
DownloadTaskInfo SampleStateAt(DownloadTaskModel& model, const DownloadTaskInfo& base, TaskState state,
							   std::int64_t current_size, std::int64_t wire_speed, std::int64_t now_ms) {
	DownloadTaskInfo fresh = base;
	fresh.set_task_state(state);
	fresh.set_task_current_size(current_size);
	fresh.set_task_download_speed(wire_speed);
	model.UpdateTaskById(fresh.task_id(), fresh, now_ms);
	return fresh;
}

DownloadTaskInfo SampleAt(DownloadTaskModel& model, const DownloadTaskInfo& base,
						  std::int64_t current_size, std::int64_t wire_speed, std::int64_t now_ms) {
	return SampleStateAt(model, base, base.task_state(), current_size, wire_speed, now_ms);
}

// 一整条时间线上的告警统计。只在"某个恰好的采样点"断言是查不出问题的:上一版护栏
// 就是只看 t=150000 这一个恰好等于重置时刻的点,换成 t=145000 立刻变红。
struct StallTimeline {
	int samples{0};
	int stalled_samples{0};
	std::int64_t first_stalled_ms{-1};
	int stalled_to_healthy_transitions{0};  // 告警一旦成立就不该再翻回"健康"
	int finite_eta_while_stalled{0};		// 报着停滞却同时给出有限 ETA = 自相矛盾
	// ETA 一旦给得出来就不该再翻回 "Unknown":健康下载上的"有限值 ↔ Unknown"来回闪
	// 与误报的橙色告警是同一个病根(把批次粒度当成了单块粒度)
	std::int64_t first_finite_eta_ms{-1};
	int unknown_eta_after_first_finite{0};
	double stalled_ratio() const {
		return samples == 0 ? 0.0 : static_cast<double>(stalled_samples) / samples;
	}
};

// 复刻 alloc.reset_part():bytes_done 按 bytes_per_second 爬升,累计涨满一个 PART_SIZE
// 后整段作废回到起点,如此循环 cycles 轮。这正是引擎 22ea14b 之后真实会发生的形状。
StallTimeline RunPartResetLoop(std::int64_t bytes_per_second, int cycles) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	const std::int64_t floor_size = base.task_current_size();
	const std::int64_t period_s	  = kEd2kPartSize / bytes_per_second;
	std::int64_t current		  = floor_size;
	StallTimeline timeline;
	bool was_stalled = false;
	for (std::int64_t t = 1000; t <= period_s * cycles * 1000; t += 1000) {
		current += bytes_per_second;
		if (current - floor_size >= kEd2kPartSize) current = floor_size;  // reset_part()
		SampleAt(model, base, current, kWireSpeed, t);
		const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
		if (live == nullptr) break;
		const bool stalled = live->progress_stalled();
		++timeline.samples;
		if (stalled) {
			++timeline.stalled_samples;
			if (timeline.first_stalled_ms < 0) timeline.first_stalled_ms = t;
			if (live->remaining_time() >= 0) ++timeline.finite_eta_while_stalled;
		}
		if (was_stalled && !stalled) ++timeline.stalled_to_healthy_transitions;
		was_stalled = stalled;
	}
	return timeline;
}

// 一次完全健康的慢速下载:线上速率恒定,进度按引擎真实的批次粒度(552960 字节)整块
// 跳变。任何一个告警都是误报 —— 数据一直在到达,也一直在按引擎自己的节奏落盘。
StallTimeline RunHealthySlowDownload(std::int64_t bytes_per_second, std::int64_t duration_s) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	std::int64_t landed	 = base.task_current_size();
	std::int64_t pending = 0;
	StallTimeline timeline;
	bool was_stalled = false;
	for (std::int64_t t = 1000; t <= duration_s * 1000; t += 1000) {
		pending += bytes_per_second;
		while (pending >= kEngineProgressStep) {
			pending -= kEngineProgressStep;
			landed += kEngineProgressStep;
		}
		SampleAt(model, base, landed, bytes_per_second, t);
		const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
		if (live == nullptr) break;
		const bool stalled = live->progress_stalled();
		++timeline.samples;
		if (stalled) {
			++timeline.stalled_samples;
			if (timeline.first_stalled_ms < 0) timeline.first_stalled_ms = t;
			if (live->remaining_time() >= 0) ++timeline.finite_eta_while_stalled;
		}
		if (was_stalled && !stalled) ++timeline.stalled_to_healthy_transitions;
		was_stalled = stalled;

		const bool eta_known = live->remaining_time() >= 0;
		if (eta_known && timeline.first_finite_eta_ms < 0) timeline.first_finite_eta_ms = t;
		if (!eta_known && timeline.first_finite_eta_ms >= 0) ++timeline.unknown_eta_after_first_finite;
	}
	return timeline;
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
// 一路涨到 part 末尾又被打回原点,如此循环。真实回退量是整整一个 PART_SIZE(9,728,000
// 字节),按单源 5-50 KiB/s 重下一轮要 190-2000 秒 —— 远长于 45 秒的速率窗口,于是
// 窗口整段落在爬升段内、rate_bps() 恒为正。任何"进度在涨就不算停滞"的短路条件都会
// 让水位判据彻底失效(引擎在 download.hpp 的 ProgressFn 注释里明文禁止过这条短路)。
// 判据必须只认历史最高水位:只有真正超过以往最高进度才算前进过。
TEST(DownloadTaskModelTest, RepeatedPartResetsAreReportedAsStalledAcrossTheWholeTimeline) {
	struct Case {
		std::int64_t bytes_per_second;
		const char* label;
	};
	// 三档常见单源速率。周期分别是 1900 / 475 / 190 秒,全部远超 45 秒窗口。
	const Case cases[] = {
		{5 * 1024, "5 KiB/s"},
		{20 * 1024, "20 KiB/s"},
		{50 * 1024, "50 KiB/s"},
	};

	for (const auto& c : cases) {
		const StallTimeline timeline = RunPartResetLoop(c.bytes_per_second, 5);
		ASSERT_GT(timeline.samples, 0) << c.label;
		// 第一轮爬升是货真价实的前进(水位一直在刷新),不该报警;从第二轮起水位再也
		// 越不过去,告警必须一直挂着。5 轮时间线上的理论占比 79%/76%/71%。
		EXPECT_GE(timeline.stalled_ratio(), 0.65)
			<< c.label << ": stalled " << timeline.stalled_samples << "/" << timeline.samples
			<< " samples; first alarm at " << timeline.first_stalled_ms << " ms";
		// 一旦报出停滞,重下同一个 part 的爬升不得把告警又抹掉(那会变成橙色闪烁)
		EXPECT_EQ(timeline.stalled_to_healthy_transitions, 0) << c.label;
		// 报着"没有新进展"却同时给出一个有限 ETA,是自相矛盾的两条信息
		EXPECT_EQ(timeline.finite_eta_while_stalled, 0) << c.label;
	}
}

// 非回归(本次缺陷的正面):一次完全健康的慢速下载必须全程零告警。
//
// 进度按引擎真实的批次粒度推进 —— 一批 3 × 184320 = 552960 字节。5 KiB/s 下 bytes_done
// 每 108 秒才跳一次,期间纹丝不动;而线上速率来自独立的 wire meter,全程稳定 5 KiB/s。
// 旧判据是纯墙钟的"90 秒没刷新水位就告警",于是 t=90..107 挂告警、t=108 批次到货撤下,
// 每 108 秒翻转一次(18/108 ≈ 16.7% 的时间挂着橙色)。误报阈值 = 552960/90 = 6144 B/s:
// 任何聚合速率低于 6 KiB/s 的下载一律周期性误报。
//
// 断言是"零告警",不是"大部分时间没告警" —— 这条下载没有任何一秒是不健康的。
TEST(DownloadTaskModelTest, SlowButAdvancingDownloadIsNeverReportedAsStalled) {
	struct Case {
		std::int64_t bytes_per_second;
		std::int64_t duration_s;
		const char* label;
	};
	// 四档常见单源速率。时长都取到能走完至少 6 个完整批次,让"批次间隙"反复出现:
	// 一批的间隔分别是 540 / 108 / 27 / 11 秒。
	const Case cases[] = {
		{1 * 1024, 3600, "1 KiB/s"},
		{5 * 1024, 1800, "5 KiB/s"},
		{20 * 1024, 1800, "20 KiB/s"},
		{50 * 1024, 1800, "50 KiB/s"},
	};

	for (const auto& c : cases) {
		const StallTimeline timeline = RunHealthySlowDownload(c.bytes_per_second, c.duration_s);
		ASSERT_GT(timeline.samples, 0) << c.label;
		EXPECT_EQ(timeline.stalled_samples, 0)
			<< c.label << ": healthy download flagged stalled on " << timeline.stalled_samples << "/"
			<< timeline.samples << " samples (" << timeline.stalled_ratio() * 100.0
			<< "%), first alarm at " << timeline.first_stalled_ms << " ms";
		// ETA 也不许在健康下载上"有限值 ↔ Unknown"来回闪:批次间隙里速率窗口一个批次
		// 都装不下时,旧实现每隔一段就把 ETA 打回 Unknown
		EXPECT_GE(timeline.first_finite_eta_ms, 0) << c.label << ": ETA never became known";
		EXPECT_EQ(timeline.unknown_eta_after_first_finite, 0)
			<< c.label << ": ETA flickered back to Unknown " << timeline.unknown_eta_after_first_finite
			<< " times after first becoming known";
	}
}

// 回归:误报的临界区就在"聚合速率低于 6144 B/s"这一侧(552960 / 90 秒)。旧判据下
// 速率越低误报占比越高 —— 1 KiB/s 时高达 83% 的时间挂着告警。这一条把临界值两侧都扫一遍。
TEST(DownloadTaskModelTest, HealthyDownloadBelowTheOldStallCliffIsNeverReportedAsStalled) {
	// 旧实现的误报阈值:一批 552960 字节 / 90 秒墙钟
	constexpr std::int64_t kOldFalseAlarmCliffBps = kEngineProgressStep / 90;
	static_assert(kOldFalseAlarmCliffBps == 6144, "the false-alarm cliff is 6 KiB/s");

	const std::int64_t rates[] = {512, 1024, 2048, 4096, 5120, kOldFalseAlarmCliffBps - 1,
								  kOldFalseAlarmCliffBps, kOldFalseAlarmCliffBps + 1};
	for (const std::int64_t bps : rates) {
		// 至少走完 3 个批次
		const std::int64_t duration_s = kEngineProgressStep / bps * 3 + 120;
		const StallTimeline timeline  = RunHealthySlowDownload(bps, duration_s);
		ASSERT_GT(timeline.samples, 0) << bps << " B/s";
		EXPECT_EQ(timeline.stalled_samples, 0)
			<< bps << " B/s: healthy download flagged stalled on " << timeline.stalled_samples << "/"
			<< timeline.samples << " samples (" << timeline.stalled_ratio() * 100.0
			<< "%), first alarm at " << timeline.first_stalled_ms << " ms";
	}
}

// 暂停期间引擎照样每秒推送快照(ed2k 的 1s 采样只过滤 completed/failed/cancelled,
// paused 照发;BrowserManager 的 kPause 分支把行留在 active_model_ 并走 UpdateTaskById)。
// 这些平样本既不能污染速率窗口,也不能推着停滞时钟往前走 —— 否则一次完全健康的恢复
// 会在瞬间满足"停滞 90 秒以上",橙色告警立刻弹出来。
TEST(DownloadTaskModelTest, PauseThenResumeIsNeverReportedAsStalled) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	// 恢复前先正常下 30 秒:每 10 秒落一个 AICH 块
	std::int64_t current = base.task_current_size();
	std::int64_t t		 = 1000;
	for (; t <= 30000; t += 1000) {
		if (t % 10000 == 0) current += kAichBlock;
		SampleAt(model, base, current, kWireSpeed, t);
	}

	// 用户暂停 5 分钟:进度与速度都停在原地
	for (; t <= 330000; t += 1000) {
		SampleStateAt(model, base, TaskState::kPause, current, 0, t);
	}

	// 恢复的第一帧就必须是健康的
	SampleStateAt(model, base, TaskState::kActive, current, kWireSpeed, t);
	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	EXPECT_FALSE(live->progress_stalled()) << "a healthy resume must not raise a stall warning";
	const std::int64_t resume_ms = t;
	t += 1000;

	// 恢复后按原速继续,整段都不许报停滞
	for (; t <= resume_ms + 90000; t += 1000) {
		if (t % 10000 == 0) current += kAichBlock;
		SampleStateAt(model, base, TaskState::kActive, current, kWireSpeed, t);
		live = model.GetTaskById(base.task_id());
		ASSERT_NE(live, nullptr);
		ASSERT_FALSE(live->progress_stalled()) << "resumed download flagged stalled at t=" << t;
		// 恢复满 20 秒后窗口里本该只剩恢复后的样本;若暂停期的平样本还留在窗口里,
		// 速率会被稀释成真值的零头,ETA 随之虚高数倍
		if (t >= resume_ms + 20000) {
			const std::int64_t expected_rate = kAichBlock / 10;
			ASSERT_NEAR(static_cast<double>(live->progress_rate_bps()),
						static_cast<double>(expected_rate), expected_rate * 0.3)
				<< "progress rate diluted by paused samples at t=" << t;
		}
	}
}

// 慢速点滴源:每 2 秒才到一个子帧,于是一半的 1 秒采样上线速为 0,平均 5 KiB/s。
// 判据若逐帧依赖"此刻线上速度 > 0",告警会以 1 秒为周期明灭,用户看到的是闪烁的橙色。
// 这条时间线上 bytes_done 从头到尾一个字节都没动 —— 是真停滞,必须报,而且报出来之后
// 不许再翻回"健康"。断言不写死告警出现的时刻(那样等于把阈值抄进测试),只查"一旦
// 报出就一直挂着"。
TEST(DownloadTaskModelTest, StallWarningIsSteadyWhenTheWireDeliversInBursts) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	int transitions			   = 0;
	int healthy_after_stalled  = 0;
	std::int64_t first_stalled = -1;
	bool was_stalled		   = false;
	// 平均 5 KiB/s 的点滴源,跑 20 分钟:足够任何合理的判据(墙钟的或字节的)完成判定
	for (std::int64_t t = 1000; t <= 1200000; t += 1000) {
		const std::int64_t wire = (t / 1000) % 2 == 0 ? 10 * 1024 : 0;
		SampleAt(model, base, base.task_current_size(), wire, t);
		const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
		ASSERT_NE(live, nullptr);
		const bool stalled = live->progress_stalled();
		if (stalled && first_stalled < 0) first_stalled = t;
		// 第一次翻成"停滞"是应该的,只统计它之后还有没有再翻动
		if (first_stalled >= 0 && t > first_stalled) {
			if (!stalled) ++healthy_after_stalled;
			if (stalled != was_stalled) ++transitions;
		}
		was_stalled = stalled;
	}

	ASSERT_GE(first_stalled, 0) << "a transfer that receives 20 minutes of data and writes none must warn";
	EXPECT_EQ(healthy_after_stalled, 0) << "stall warning flickers with a bursty source";
	EXPECT_EQ(transitions, 0);
	// 线上一直有数据到达,所以走的是"一个字节都没落盘"那一支
	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	EXPECT_EQ(static_cast<int>(live->stall_kind()), 1);
}

// 两种局面在界面上必须说不同的话:
//   (a) 一个字节都存不下来(空闲超时把未凑齐的区间整段丢弃)—— bytes_done 纹丝不动;
//   (b) 数据确实落了盘,只是整个 part 的 MD4 没过被作废重下 —— bytes_done 涨了又退。
// (b) 里说 "none of it can be saved" 是错的,所以必须有一个能把两者分开的信号。
TEST(DownloadTaskModelTest, StallKindSeparatesNothingLandedFromWrittenThenDiscarded) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	const auto roles	= model.roleNames();
	const int kind_role = roles.key(QByteArray("progressStallKind"), -1);
	ASSERT_NE(kind_role, -1) << "the UI cannot tell the two stall shapes apart without this role";

	// (a) 一个字节都没落盘
	for (std::int64_t t = 1000; t <= 150000; t += 1000) {
		SampleAt(model, base, base.task_current_size(), kWireSpeed, t);
	}
	const QModelIndex row = model.index(0, 0);
	EXPECT_TRUE(model.data(row, DownloadTaskModel::kTaskProgressStalled).toBool());
	EXPECT_EQ(model.data(row, kind_role).toInt(), 1) << "nothing landed at all";

	// (b) 落了盘又被整段作废重下
	DownloadTaskModel reset_model;
	reset_model.AddTask(base, 0);
	const std::int64_t floor_size = base.task_current_size();
	constexpr std::int64_t kBps	  = 50 * 1024;
	const std::int64_t period_s	  = kEd2kPartSize / kBps;
	std::int64_t current		  = floor_size;
	for (std::int64_t t = 1000; t <= period_s * 3 * 1000; t += 1000) {
		current += kBps;
		if (current - floor_size >= kEd2kPartSize) current = floor_size;
		SampleAt(reset_model, base, current, kWireSpeed, t);
	}
	const QModelIndex reset_row = reset_model.index(0, 0);
	EXPECT_TRUE(reset_model.data(reset_row, DownloadTaskModel::kTaskProgressStalled).toBool());
	EXPECT_EQ(reset_model.data(reset_row, kind_role).toInt(), 2)
		<< "data did land on disk, it was discarded afterwards";
}

// 多 part 文件里某个 part 被 reset_part() 作废、其余 part 仍在正常推进:bytes_done 会
// 一直上涨,只是要花很久才重新涨过历史最高水位(5 KiB/s 下重新挣回一个 PART_SIZE 要
// 约 32 分钟)。这段时间里净进度相对水位确实 ≤ 0,报停滞是对的,归到"落了盘又被撤销"
// 也是对的 —— 但绝不能落进"一个字节都没落盘":数据一直在写,那句话是假的。
// 注意时间判据在这里同样不够用:进度每 108 秒才推进一批,"距上次增长 ≥ 90 秒"在每个
// 批次间隙里都会成立,只有"自上次增长以来线上收够了一整份预算"才分得开这两种事实。
TEST(DownloadTaskModelTest, ResetPartWhileOtherPartsAdvanceIsWrittenThenDiscarded) {
	DownloadTaskModel model;
	const DownloadTaskInfo base = MakeEd2kActiveTask();
	model.AddTask(base, 0);

	constexpr std::int64_t kBps = 5 * 1024;
	std::int64_t landed			= base.task_current_size();
	std::int64_t pending		= 0;
	std::int64_t t				= 1000;

	// 先健康地下 20 分钟,把历史水位抬上去
	for (; t <= 1200000; t += 1000) {
		pending += kBps;
		while (pending >= kEngineProgressStep) {
			pending -= kEngineProgressStep;
			landed += kEngineProgressStep;
		}
		SampleAt(model, base, landed, kBps, t);
	}
	const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
	ASSERT_NE(live, nullptr);
	ASSERT_FALSE(live->progress_stalled()) << "the healthy warm-up must not warn";

	// 某个 part 的 MD4 没过,被整段作废
	landed -= kEd2kPartSize;
	SampleAt(model, base, landed, kBps, t);
	t += 1000;

	// 其余 part 照常按批推进,但 25 分钟内都涨不回原来的水位
	int nothing_lands_samples = 0;
	int stalled_samples		  = 0;
	int samples				  = 0;
	for (; t <= 1200000 + 1500000; t += 1000) {
		pending += kBps;
		while (pending >= kEngineProgressStep) {
			pending -= kEngineProgressStep;
			landed += kEngineProgressStep;
		}
		SampleAt(model, base, landed, kBps, t);
		live = model.GetTaskById(base.task_id());
		ASSERT_NE(live, nullptr);
		++samples;
		if (live->progress_stalled()) ++stalled_samples;
		if (live->stall_kind() == StallKind::kNothingLands) ++nothing_lands_samples;
	}

	ASSERT_GT(samples, 0);
	EXPECT_GT(stalled_samples, 0) << "progress never regains its high-water mark; that must be reported";
	EXPECT_EQ(nothing_lands_samples, 0)
		<< "data kept landing on disk; the warning must not claim none of it is being written ("
		<< nothing_lands_samples << "/" << samples << " samples said so)";
	EXPECT_EQ(static_cast<int>(live->stall_kind()), 2);
}

// ============================================================================
// 恢复/重试路径的粒度污染:字节预算的"实测粒度"绝不能被续传跳变钉死
//
// 引擎在下载开始时 sync_progress()(download.cpp:3443),续传任务的整个已恢复字节数
// 在一次回调里到达;而桌面侧此前 connecting 阶段的行早已以 current_size = 0 存在了
// 好几秒(browser_manager 的状态事件建行 + 1Hz 采样都报 done = 0)。于是 0 到 120 MiB
// 是一次单样本增长。粒度若采信它,预算被钉在 64 MiB 上限、终生不降(粒度只增不减,
// 只有 Restart 清它,而 Restart 只在暂停/时钟倒流时触发)。
// 触发人群:RestoreEd2kDownloadHistory(每次启动重加全部 kError 记录)、RetryTask、
// 手动重加同链接 —— 正是最可能再次卡住、最需要告警的任务。
// ============================================================================

// 缺陷复现:续传 120 MiB 跳变之后接一个半死源(线上恒 96 KiB/s,每 600 秒才真正落
// 一批 552960 字节,即 99.2% 的接收被丢弃)。预算必须回到 2 MiB 下限:线上 21 秒即
// 凑满预算,墙钟 90 秒是更晚的那道门,首告警 = 跳变后 90 秒。若跳变污染了粒度,
// 预算 = 64 MiB,而每个 600 秒周期线上只到 56.25 MiB,永远凑不够 => 永不告警,
// 700 MiB 的文件要按 5 个月往后拖,期间界面一直装作一切正常。
TEST(DownloadTaskModelTest, ResumeJumpMustNotPinTheStallBudgetOnAHalfDeadSource) {
	DownloadTaskModel model;
	DownloadTaskInfo base = MakeEd2kActiveTask();
	base.set_task_current_size(0);
	model.AddTask(base, 0);

	// 连接阶段:行以 current_size = 0 存在了好几秒
	std::int64_t t = 1000;
	for (; t <= 5000; t += 1000) {
		SampleAt(model, base, 0, 0, t);
	}

	// 续传同步:整个已恢复的 120 MiB 在一次采样里到达
	std::int64_t current = 120LL * 1024 * 1024;
	SampleAt(model, base, current, kWireSpeed, t);
	const std::int64_t jump_ms = t;
	t += 1000;

	// 半死源:两小时时间线,每 600 秒落一批,线上恒 96 KiB/s
	StallTimeline timeline;
	bool was_stalled = false;
	for (; t <= jump_ms + 7200000; t += 1000) {
		if ((t - jump_ms) % 600000 == 0) current += kEngineProgressStep;
		SampleAt(model, base, current, kWireSpeed, t);
		const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
		ASSERT_NE(live, nullptr);
		const bool stalled = live->progress_stalled();
		++timeline.samples;
		if (stalled) {
			++timeline.stalled_samples;
			if (timeline.first_stalled_ms < 0) timeline.first_stalled_ms = t;
			if (live->remaining_time() >= 0) ++timeline.finite_eta_while_stalled;
		}
		if (was_stalled && !stalled) ++timeline.stalled_to_healthy_transitions;
		was_stalled = stalled;
	}

	ASSERT_GT(timeline.samples, 0);
	ASSERT_GE(timeline.first_stalled_ms, 0)
		<< "a resumed task that discards 99% of received bytes never warned in 2 hours";
	// 新告警仍是纯墙钟 90 秒规则的严格子集:不早于跳变后 90 秒
	EXPECT_GE(timeline.first_stalled_ms, jump_ms + 90000);
	EXPECT_LE(timeline.first_stalled_ms, jump_ms + 120000)
		<< "the stall budget is still inflated by the resume jump";
	// 每个 600 秒周期里,告警应从第 90 秒挂到批次落地(510/600 = 85%)
	EXPECT_GE(timeline.stalled_ratio(), 0.7)
		<< "stalled " << timeline.stalled_samples << "/" << timeline.samples << " samples";
	// 报着停滞就不许同时给出有限 ETA
	EXPECT_EQ(timeline.finite_eta_while_stalled, 0);
}

// 性质:全新任务从 0 起步按批推进,行为与修复前完全一致 —— 粒度 552960、预算落在
// 2 MiB 下限(552960 x 3 < 2 MiB)。健康爬升段零告警;冻结后线上 21 秒凑满预算,
// 墙钟 90 秒是更晚的那道门,首告警恰在最后一次推进后 90 秒。
TEST(DownloadTaskModelTest, FreshTaskFromZeroKeepsTheFloorBudgetBehavior) {
	DownloadTaskModel model;
	DownloadTaskInfo base = MakeEd2kActiveTask();
	base.set_task_current_size(0);
	model.AddTask(base, 0);

	// 健康推进 10 批:96 KiB/s 下一批约 5.6 秒,取每 6 秒一批
	std::int64_t current = 0;
	std::int64_t t		 = 1000;
	for (; t <= 60000; t += 1000) {
		if (t % 6000 == 0) current += kEngineProgressStep;
		SampleAt(model, base, current, kWireSpeed, t);
		const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
		ASSERT_NE(live, nullptr);
		ASSERT_FALSE(live->progress_stalled()) << "healthy ramp-up flagged stalled at t=" << t;
	}
	const std::int64_t last_advance_ms = 60000;

	// 冻结:线上照收,一个字节不再落盘
	std::int64_t first_stalled = -1;
	for (; t <= last_advance_ms + 300000; t += 1000) {
		SampleAt(model, base, current, kWireSpeed, t);
		const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
		ASSERT_NE(live, nullptr);
		if (live->progress_stalled()) {
			first_stalled = t;
			break;
		}
	}
	ASSERT_GE(first_stalled, 0) << "a frozen download with wire data flowing must warn";
	EXPECT_GE(first_stalled, last_advance_ms + 90000);
	EXPECT_LE(first_stalled, last_advance_ms + 100000);
}

// 性质:只推进过一次就冻结的任务,粒度还测不出来,预算必须落在 2 MiB 下限 ——
// 保守、告警更快;健康间隙线上只会积到一个粒度(552960 < 2 MiB),不会因此误报。
TEST(DownloadTaskModelTest, SingleAdvanceThenFreezeAlarmsAtTheFloorBudget) {
	DownloadTaskModel model;
	DownloadTaskInfo base = MakeEd2kActiveTask();
	base.set_task_current_size(0);
	model.AddTask(base, 0);

	// 起步 10 秒还没有任何推进
	std::int64_t t = 1000;
	for (; t <= 10000; t += 1000) {
		SampleAt(model, base, 0, kWireSpeed, t);
	}
	// 仅有的一次推进:一批 552960
	SampleAt(model, base, kEngineProgressStep, kWireSpeed, t);
	const std::int64_t only_advance_ms = t;
	t += 1000;

	// 此后冻结,线上照收
	std::int64_t first_stalled = -1;
	for (; t <= only_advance_ms + 300000; t += 1000) {
		SampleAt(model, base, kEngineProgressStep, kWireSpeed, t);
		const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
		ASSERT_NE(live, nullptr);
		if (live->progress_stalled()) {
			first_stalled = t;
			break;
		}
	}
	ASSERT_GE(first_stalled, 0);
	EXPECT_GE(first_stalled, only_advance_ms + 90000);
	EXPECT_LE(first_stalled, only_advance_ms + 100000);
}

// 兜底:粒度已被正确测出之后,下载中途一次荒谬的单样本跳变(断线重连后的整段重
// 同步、事件线程被饿死把几十批合并进一次采样)也不许被采信 —— 它不是任何合理的
// 流水线批次(当前一批 552960 字节,8 MiB 已是它的 14 倍,也小于一个 PART_SIZE)。
// 若被采信,40 MiB x 3 触顶 64 MiB,后续半死源每 600 秒只送 56.25 MiB,永不告警。
TEST(DownloadTaskModelTest, MidDownloadJumpBeyondAnySaneBatchDoesNotInflateTheBudget) {
	DownloadTaskModel model;
	DownloadTaskInfo base = MakeEd2kActiveTask();
	base.set_task_current_size(0);
	model.AddTask(base, 0);

	// 先健康地按批下 60 秒,粒度已被正确测出(552960)
	std::int64_t current = 0;
	std::int64_t t		 = 1000;
	for (; t <= 60000; t += 1000) {
		if (t % 6000 == 0) current += kEngineProgressStep;
		SampleAt(model, base, current, kWireSpeed, t);
	}

	// 一次采样里 bytes_done 跳了 40 MiB
	current += 40LL * 1024 * 1024;
	SampleAt(model, base, current, kWireSpeed, t);
	const std::int64_t jump_ms = t;
	t += 1000;

	// 之后退化成半死源:每 600 秒才落一批,线上恒 96 KiB/s
	std::int64_t first_stalled = -1;
	for (; t <= jump_ms + 7200000; t += 1000) {
		if ((t - jump_ms) % 600000 == 0) current += kEngineProgressStep;
		SampleAt(model, base, current, kWireSpeed, t);
		const DownloadTaskInfo* live = model.GetTaskById(base.task_id());
		ASSERT_NE(live, nullptr);
		if (live->progress_stalled()) {
			first_stalled = t;
			break;
		}
	}
	ASSERT_GE(first_stalled, 0)
		<< "a half-dead source after a mid-download jump never warned in 2 hours";
	EXPECT_GE(first_stalled, jump_ms + 90000);
	EXPECT_LE(first_stalled, jump_ms + 120000)
		<< "the absurd 40 MiB step was believed as landing granularity";
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
