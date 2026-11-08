#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QEvent>
#include <QTemporaryDir>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include "Browser/browser_manager.h"
#include "Browser/download_task_utils.h"
#include "cache/cache.h"
#include "ed2k_engine_def.h"

namespace gdl::ui::browser {
	// 只开放真实生产入口给回归测试，不替换持久化、解析、模型或引擎实现。
	class BrowserPersistenceTestPeer {
	   public:
		static std::unique_ptr<BrowserManagerImpl> Create() {
			return std::unique_ptr<BrowserManagerImpl>(new BrowserManagerImpl());
		}
		static void Restore(BrowserManagerImpl& browser) {
			browser.InitDownloadHistoryCache();
			browser.RestoreEd2kDownloadHistory();
		}
		static std::uint64_t Generation(BrowserManagerImpl& browser, const QString& id) {
			return browser.ed2k_generations_.value(id);
		}
		static void Progress(BrowserManagerImpl& browser, const nlohmann::json& value) {
			// 模拟真实 PubSub 网络线程入口，确保测试经过 invokeMethod 的跨线程队列。
			std::thread worker([&] { browser.OnHandleEd2kActiveProgress(value.dump()); });
			worker.join();
		}
		static void State(BrowserManagerImpl& browser, const nlohmann::json& value) {
			std::thread worker([&] { browser.OnHandleEd2kTaskState(value.dump()); });
			worker.join();
		}
		static void Aria(BrowserManagerImpl& browser, const std::string& msg) { browser.OnHandleAria2Message(msg); }
	};
}  // namespace gdl::ui::browser

using namespace gdl::ui::browser;
namespace {
	void Drain() {
		QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
	}
	const QString kHash = QStringLiteral("0123456789abcdef0123456789abcdef");
	const QString kId	= QStringLiteral("ed2k-") + kHash;
	const QString kLink = QStringLiteral("ed2k://|file|resume.bin|4096|") + kHash + QStringLiteral("|/");

	auto Record() {
		return gdl::cache::DownloadHistoryCache::Instance().GetRecord(kId.toStdString());
	}
}  // namespace

TEST(Ed2kPersistence, RealBrowserCacheAndEngineLifecycle) {
	int argc	 = 1;
	char name[]	 = "ed2k-persistence";
	char* argv[] = {name, nullptr};
	QCoreApplication app(argc, argv);
	QTemporaryDir temp;
	ASSERT_TRUE(temp.isValid());
	auto& history = gdl::cache::DownloadHistoryCache::Instance();
	const auto db = (temp.path() + "/history.db").toStdString();
	ASSERT_TRUE(history.Initialize(db).IsOk());
	auto& engine = gdl::engine::Ed2kDownloadManager::Instance();
	gdl::engine::Ed2kDownloadManager::Ed2kEngineConfig config;
	config.data_dir				= (temp.path() + "/ed2k").toStdString();
	config.tcp_port				= 0;
	config.udp_port				= 0;
	config.max_concurrent_tasks = 0;
	config.diag_level			= "off";
	ASSERT_TRUE(engine.InitEd2kEngine(config));
	auto browser = BrowserPersistenceTestPeer::Create();
	// 单独见证真实引擎的初始 PubSub 状态；模型注入用固定字节数，排除周期采样的时间竞争。
	std::promise<nlohmann::json> initial;
	auto initial_future = initial.get_future();
	std::atomic_bool seen{false};
	auto witness = engine.SubscriptionEd2kMessage(kEd2kActiveProgress, [&](const std::string& msg) {
		const auto items = nlohmann::json::parse(msg);
		for (const auto& item : items) {
			if (item.value("id", std::string()) == kId.toStdString() && !seen.exchange(true)) initial.set_value(item);
		}
	});
	// 初始 PubSub 见证使用引用捕获，守卫先于这些状态析构，确保断言早退也完成线程回收。
	struct EngineGuard {
		gdl::engine::Ed2kDownloadManager& engine;
		std::shared_ptr<std::promise<void>> release;
		void Release() {
			if (release) {
				engine.PauseAll();
				release->set_value();
				release.reset();
			}
		}
		~EngineGuard() { Release(); engine.ShutdownEngine(); }
	} guard{engine, {}};
	const QVariantMap options{{"dir", temp.path()}, {"pause", true}};
	ASSERT_TRUE(browser->AddEd2kTask({kLink}, options));
	// 创建返回时已落库；未经过事件循环也可退出恢复。
	ASSERT_TRUE(Record().IsOk());
	ASSERT_TRUE(Record().Value().has_value());
	EXPECT_EQ(Record().Value()->state, gdl::cache::DownloadState::kPause);
	ASSERT_EQ(initial_future.wait_for(std::chrono::seconds(5)), std::future_status::ready);
	const auto first = initial_future.get();
	EXPECT_EQ(first.value("state", std::string()), "paused");
	engine.UnSubscribeEd2kMessage(witness);
	// 暂挂网络线程使提交顺序确定；放行后的真实连接和有界关停也必须正常析构。
	auto entered = std::make_shared<std::promise<void>>();
	auto entered_future = entered->get_future();
	auto release = std::make_shared<std::promise<void>>();
	auto release_future = release->get_future().share();
	guard.release = release;
	auto gate = engine.SubscriptionEd2kMessage(kEd2kServerList, [entered, release_future](const std::string&) {
		entered->set_value();
		release_future.wait();
	});
	engine.RequestServerList();
	ASSERT_EQ(entered_future.wait_for(std::chrono::seconds(5)), std::future_status::ready);
	engine.UnSubscribeEd2kMessage(gate);
	const auto generation = BrowserPersistenceTestPeer::Generation(*browser, kId);
	auto progress		  = nlohmann::json::array({{{"id", kId.toStdString()},
													{"generation", generation},
													{"name", "resume.bin"},
													{"total", 4096},
													{"done", 2048},
													{"state", "downloading"},
													{"out_path", (temp.path() + "/resume.bin").toStdString()}}});
	BrowserPersistenceTestPeer::Progress(*browser, progress);
	Drain();
	EXPECT_EQ(Record().Value()->state, gdl::cache::DownloadState::kActive);
	EXPECT_EQ(Record().Value()->downloaded_size, 2048);
	BrowserPersistenceTestPeer::State(*browser, {{"id", kId.toStdString()},
												 {"generation", generation},
												 {"state", "paused"},
												 {"bytes_done", 2048},
												 {"total_size", 4096}});
	Drain();
	EXPECT_EQ(Record().Value()->state, gdl::cache::DownloadState::kPause);
	// aria2 空快照只更新自身任务，不得清空共享模型里的 ed2k 行。
	BrowserPersistenceTestPeer::Aria(*browser, R"({"result":[]})");
	Drain();
	ASSERT_NE(browser->GetActiveDownloadModel()->GetTaskById(kId), nullptr);
	browser.reset();
	ASSERT_TRUE(history.Uninitialize().IsOk());
	ASSERT_TRUE(history.Initialize(db).IsOk());
	// 模拟进程内 Session 丢失映射：取消旧任务后重新提交，目标文件尚不存在。
	ASSERT_TRUE(engine.RemoveTask(kId.toStdString(), false));
	browser = BrowserPersistenceTestPeer::Create();
	BrowserPersistenceTestPeer::Restore(*browser);
	auto* restored = browser->GetActiveDownloadModel()->GetTaskById(kId);
	ASSERT_NE(restored, nullptr);
	EXPECT_EQ(restored->task_state(), TaskState::kPause);
	EXPECT_EQ(restored->task_current_size(), 2048);
	EXPECT_EQ(Record().Value()->downloaded_size, 2048);
	// 恢复后引擎首次零进度暂停快照不得抹掉历史进度。
	const auto restored_generation = BrowserPersistenceTestPeer::Generation(*browser, kId);
	progress[0]["generation"]	   = restored_generation;
	progress[0]["state"]		   = "paused";
	progress[0]["done"]			   = 0;
	BrowserPersistenceTestPeer::Progress(*browser, progress);
	Drain();
	EXPECT_EQ(Record().Value()->downloaded_size, 2048);
	// 删除与已排队采样/终态交错：撤销代际后，两类旧消息都不应复活任务。
	BrowserPersistenceTestPeer::Progress(*browser, progress);
	const auto removed = browser->RemoveTask(0, kId, false);
	EXPECT_TRUE(removed.value("taskRemoved").toBool());
	BrowserPersistenceTestPeer::State(
		*browser, {{"id", kId.toStdString()}, {"generation", restored_generation}, {"state", "cancelled"}});
	Drain();
	EXPECT_FALSE(Record().Value().has_value());
	EXPECT_FALSE(browser->GetActiveDownloadModel()->ContainsTask(kId));
	EXPECT_FALSE(browser->GetStopedDownloadModel()->ContainsTask(kId));
	ASSERT_TRUE(browser->AddHttpTask({kLink}, {{"dir", temp.path()}}));
	EXPECT_EQ(Record().Value()->state, gdl::cache::DownloadState::kWaiting);
	const auto new_generation = BrowserPersistenceTestPeer::Generation(*browser, kId);
	EXPECT_NE(new_generation, restored_generation);
	BrowserPersistenceTestPeer::Progress(*browser, progress);
	Drain();
	EXPECT_EQ(Record().Value()->state, gdl::cache::DownloadState::kWaiting);
	// 终态撤销代际，已经转投的旧进度不能将完成记录重新变成活动态。
	BrowserPersistenceTestPeer::State(
		*browser,
		{{"id", kId.toStdString()}, {"generation", new_generation}, {"state", "completed"}, {"total_size", 4096}});
	progress[0]["generation"] = new_generation;
	BrowserPersistenceTestPeer::Progress(*browser, progress);
	Drain();
	EXPECT_EQ(Record().Value()->state, gdl::cache::DownloadState::kComplete);
	EXPECT_EQ(Record().Value()->downloaded_size, 4096);
	// 失败重试复用哈希ID：Submit已写入的新waiting记录不能被Retry旧记录清理删掉。
	ASSERT_TRUE(browser->RemoveStopTask(kId, false).value("taskRemoved").toBool());
	ASSERT_TRUE(browser->AddEd2kTask({kLink}, options));
	const auto retry_generation = BrowserPersistenceTestPeer::Generation(*browser, kId);
	ASSERT_TRUE(engine.RemoveTask(kId.toStdString(), false));
	BrowserPersistenceTestPeer::State(*browser, {{"id", kId.toStdString()},
												 {"generation", retry_generation},
												 {"state", "failed"},
												 {"bytes_done", 512},
												 {"total_size", 4096}});
	Drain();
	ASSERT_TRUE(browser->RetryTask(kId));
	ASSERT_TRUE(Record().Value().has_value());
	EXPECT_EQ(Record().Value()->state, gdl::cache::DownloadState::kWaiting);
	EXPECT_FALSE(browser->GetStopedDownloadModel()->ContainsTask(kId));
	ASSERT_TRUE(browser->RemoveTask(1, kId, false).value("taskRemoved").toBool());

	ASSERT_TRUE(browser->AddEd2kTask({kLink}, options));
	const auto shutdown_generation = BrowserPersistenceTestPeer::Generation(*browser, kId);
	// 实际 SQLite 历史的启动矩阵：活动/排队/失败重新入队，暂停不启动，完成和删除不恢复。
	const std::vector<gdl::cache::DownloadState> states{
		gdl::cache::DownloadState::kActive, gdl::cache::DownloadState::kWaiting,  gdl::cache::DownloadState::kError,
		gdl::cache::DownloadState::kPause,	gdl::cache::DownloadState::kComplete, gdl::cache::DownloadState::kRemoved};
	std::vector<QString> ids;
	for (std::size_t index = 0; index < states.size(); ++index) {
		const QString hash	= QStringLiteral("%1").arg(index + 1, 32, 16, QLatin1Char('0'));
		const QString id	= QStringLiteral("ed2k-") + hash;
		auto record			= DownloadRecordFromTaskInfo(*browser->GetActiveDownloadModel()->GetTaskById(kId));
		record.task_id		= id.toStdString();
		record.download_url = QStringLiteral("ed2k://|file|matrix%1.bin|4096|%2|/").arg(index).arg(hash).toStdString();
		record.save_path	= (temp.path() + QStringLiteral("/matrix%1.bin").arg(index)).toStdString();
		record.state		= states[index];
		record.downloaded_size = 512;
		ASSERT_TRUE(history.AddRecord(record).IsOk());
		ids.push_back(id);
	}
	BrowserPersistenceTestPeer::Restore(*browser);
	for (std::size_t index = 0; index < states.size(); ++index) {
		const auto persisted = history.GetRecord(ids[index].toStdString());
		ASSERT_TRUE(persisted.IsOk());
		if (index < 4) {
			ASSERT_TRUE(persisted.Value().has_value());
			EXPECT_NE(BrowserPersistenceTestPeer::Generation(*browser, ids[index]), 0);
			EXPECT_EQ(persisted.Value()->downloaded_size, 512);
			EXPECT_EQ(persisted.Value()->state,
					  index == 3 ? gdl::cache::DownloadState::kPause : gdl::cache::DownloadState::kWaiting);
		}
		else {
			EXPECT_EQ(BrowserPersistenceTestPeer::Generation(*browser, ids[index]), 0);
		}
	}
	// 检查真实 PubSub 最终采样仍然在网络线程排空时发布。
	auto final_sample = std::make_shared<std::promise<nlohmann::json>>();
	auto final_future = final_sample->get_future();
	auto final_seen = std::make_shared<std::atomic_bool>(false);
	auto final_witness = engine.SubscriptionEd2kMessage(kEd2kActiveProgress, [final_sample, final_seen](const std::string& msg) {
		for (const auto& item : nlohmann::json::parse(msg)) {
			if (item.value("id", std::string()) == kId.toStdString() && !final_seen->exchange(true))
				final_sample->set_value(item);
		}
	});
	// 优雅关停必须排空已转投的暂停通知，且 shutdown 取消事件不得覆盖为 removed。
	BrowserPersistenceTestPeer::State(
		*browser,
		{{"id", kId.toStdString()}, {"generation", shutdown_generation}, {"state", "paused"}, {"bytes_done", 1024}});
	guard.Release();
	browser->UnInit();
	ASSERT_EQ(final_future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
	EXPECT_EQ(final_future.get().value("state", std::string()), "paused");
	ASSERT_TRUE(Record().Value().has_value());
	EXPECT_EQ(Record().Value()->state, gdl::cache::DownloadState::kPause);
	EXPECT_EQ(Record().Value()->downloaded_size, 1024);
	browser.reset();
	ASSERT_TRUE(history.Uninitialize().IsOk());
}
