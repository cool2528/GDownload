#include "ed2k_download_manager.h"

#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <map>
#include <optional>
#include <thread>
#include <variant>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>

#include <nlohmann/json.hpp>

#include "logger.h"

#include <ed2k/kad/messages.hpp>
#include <ed2k/link/ed2k_link.hpp>
#include <ed2k/net/runtime.hpp>
#include <ed2k/peer/c2c_connection.hpp>
#include <ed2k/server/opcodes.hpp>
#include <ed2k/session/session.hpp>

#include "ed2k_engine_def.h"

namespace gdl {
namespace engine {

namespace {

// 把引擎的 TaskState 枚举转换为 PubSub 事件约定的字符串
std::string TaskStateToString(ed2k::session::TaskState state) {
	using ed2k::session::TaskState;
	switch (state) {
		case TaskState::queued:
			return "queued";
		case TaskState::connecting:
			return "connecting";
		case TaskState::downloading:
			return "downloading";
		case TaskState::paused:
			return "paused";
		case TaskState::completed:
			return "completed";
		case TaskState::failed:
			return "failed";
		case TaskState::cancelled:
			return "cancelled";
	}
	return "unknown";
}

// 把一批搜索结果序列化为 kEd2kSearchResult 的 JSON payload
std::string SearchResultsToJson(const std::vector<ed2k::server::SearchResultItem>& items, bool append) {
	nlohmann::json doc;
	doc["ok"] = true;
	doc["error"] = std::string();
	doc["append"] = append;
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& item : items) {
		nlohmann::json j;
		j["name"] = item.name;
		j["size"] = item.size;
		j["hash"] = item.hash.to_hex();
		j["sources"] = item.sources;
		j["complete_sources"] = item.complete_sources;
		arr.push_back(std::move(j));
	}
	doc["items"] = std::move(arr);
	return doc.dump();
}

// 取快照里"此刻真正在连的源数"。
// 引擎的 TaskSnapshot::active_sources 是 v2.7.4 之后新增的字段, 而 vcpkg overlay port 目前仍钉在
// v2.7.4, 因此这里按"字段是否存在"探测, 而不是用 ed2k/version.hpp 的 ED2K_VERSION_AT_LEAST ——
// 引擎版本号只在发版 commit 里 bump, 用本地 worktree 重建出来的测试包版本仍写着 2.7.4 却已经带上
// 了该字段, 按版本宏判断反而会误判成"没有"。写成泛型 lambda/模板是必需的: 非模板上下文里的
// if constexpr 两个分支都会被实例化, 缺字段的那侧就会编译失败。
// port 升到带该字段的引擎版本后, 本函数可以直接换成 snap.active_sources。
template <class Snapshot>
std::int64_t ActiveSourcesOf(const Snapshot& snap) {
	if constexpr (requires { snap.active_sources; }) {
		return static_cast<std::int64_t>(snap.active_sources);
	} else {
		// 旧引擎没有该字段: 退化成已知源数, 即维持引入本改动之前的显示口径
		return static_cast<std::int64_t>(snap.known_sources);
	}
}

std::string SearchErrorToJson(const std::string& error) {
	nlohmann::json doc;
	doc["ok"] = false;
	doc["error"] = error;
	doc["append"] = false;
	doc["items"] = nlohmann::json::array();
	return doc.dump();
}

}  // namespace

// 内部实现：网络线程运行 ed2k::net::IoRuntime，Session 在其上构造/析构。
// id_to_session/session_to_id/sample_timer 只在网络线程读写（对外方法均 post 到网络线程执行）。
struct Ed2kDownloadManager::Impl {
	ed2k::net::IoRuntime runtime;
	std::unique_ptr<ed2k::session::Session> session;
	std::unique_ptr<PubSubSystem<std::string>> pubsub;
	std::thread worker;  // 跑 runtime.run() 的网络线程
	std::atomic_bool running{false};
	// 网络线程的 runtime.run() 是否已自然返回（= io_context 里所有协程都已收敛、无剩余工作）。
	// ShutdownEngine 用它做"有上限的排空等待"，见那里的注释。
	std::atomic_bool run_exited{false};
	Ed2kEngineConfig config;

	// 对外任务 ID("ed2k-<md4>") 与引擎内部 session task id(uint64) 的双向映射
	std::map<std::string, std::uint64_t> id_to_session;
	std::map<std::uint64_t, std::string> session_to_id;

	// 1s 采样定时器，仅在网络线程上 arm/cancel
	std::unique_ptr<boost::asio::steady_timer> sample_timer;

	// 前台请求 in-flight 守卫：引擎契约要求同一 Session 连接上的前台请求
	// (search/search_more/connect_server/disconnect_server 等)
	// 必须串行执行——并发的前台请求会破坏单读者 socket 模型。这里用单个共享标志
	// 覆盖所有这些方法，进行中直接丢弃新请求；仅在网络线程读写，无需加锁。
	// RequestServerList/AddServer/RemoveServer/RequestKadStatus 是纯本地操作，
	// 不经过服务器连接，不受此守卫约束。
	bool foreground_in_flight = false;

	// server.met 更新独立守卫：引擎 update_server_met 走独立的 HTTPDownload 连接，
	// 不占用服务器登录 socket，与 connect_server/search 并发是安全的（列表数据的
	// 消费方 login_with_rotation 在首个挂起点之前就同步消费完 server_met_bytes）。
	// 若与 foreground_in_flight 共用：启动自动连接在空列表时轮转 8 个内建 fallback
	// 服务器（每个默认 30s 超时）长期占用守卫，启动自动更新与用户手动"从 URL 更新"
	// 会被全部静默丢弃——全新安装永远拿不到服务器列表。故仅用独立标志防自身重入；
	// 重入请求直接忽略（进行中的更新自会发布结果）。仅网络线程读写。
	bool server_met_in_flight = false;

	// 前台守卫占用期间暂存最新分享目录请求，守卫释放时重放；只保留最新一份。
	// 启动时 ConnectServer(自动连接)先占用守卫并跨整个网络往返，紧随其后的 SetSharedDirs
	// (Ed2kManager::Init 的分享恢复)会命中忙守卫——若直接丢弃则每次启动分享都恢复不了，
	// 故在此暂存，守卫释放时由 ReleaseForegroundGuardLocked 取出重放。仅网络线程读写。
	std::optional<std::vector<std::filesystem::path>> pending_shared_dirs;
};

Ed2kDownloadManager::Ed2kDownloadManager() : impl_(std::make_unique<Impl>()) {}

Ed2kDownloadManager::~Ed2kDownloadManager() {
	ShutdownEngine();
}

bool Ed2kDownloadManager::InitEd2kEngine(const Ed2kEngineConfig& config) {
	if (impl_->running.load()) {
		return true;
	}
	// 注意:ShutdownEngine 之后不支持再次 Init——runtime 内的 io_context 已 stop 且未 restart,
	// 重新 Init 后 run() 会立即返回导致引擎静默失效。当前生命周期只在进程退出时 shutdown,
	// 若将来引入"重启引擎"(如设置变更后)需先重建 Impl 或调用 io_context::restart()。
	try {
		impl_->config = config;

		// PubSub 需要一个 io_context 跑 handler 派发，复用 runtime 的 context
		impl_->pubsub = std::make_unique<PubSubSystem<std::string>>(impl_->runtime.context());

		ed2k::session::SessionConfig scfg;
		scfg.nickname = config.nickname;
		scfg.tcp_port = config.tcp_port;
		scfg.kad_udp_port = config.udp_port;
		scfg.data_dir = std::filesystem::path(config.data_dir);
		scfg.max_concurrent_tasks = config.max_concurrent_tasks;
		scfg.enable_kad = config.enable_kad;
		// 持久 UserHash:远端按 hash 记队列等待/credit,必须每安装唯一且跨启动稳定
		if (!config.user_hash_hex.empty()) {
			if (auto parsed = ed2k::UserHash::from_hex(config.user_hash_hex)) {
				scfg.user_hash = *parsed;
			}
		}
		// 下载编排预算:与引擎 CLI 验证口径一致(300s)。默认 60s 会压缩 setup/排队窗口
		scfg.task_io_timeout = std::chrono::seconds(300);
		// preferred：优先使用混淆但不强制，兼容未启用混淆的对端；required 会拒绝所有非混淆连接过于激进
		scfg.obfuscation = config.enable_obfuscation ? ed2k::peer::ObfuscationPolicy::preferred
													  : ed2k::peer::ObfuscationPolicy::disabled;

		// Session 必须在网络线程构造/使用；这里在 worker 线程启动前构造（此时尚无并发），
		// 之后所有对外访问都需要 post 到 worker 线程执行。
		// 构造可能因端口被占用(与 eMule 共存/双实例)等原因抛异常，由本函数末尾统一兜底。
		impl_->session = std::make_unique<ed2k::session::Session>(impl_->runtime, scfg);

		// 任务状态突变事件：反查 session_to_id 转为对外 task_id，转发到 PubSub。
		// 契约要求 handler 不得抛出异常，也不得在回调内反调 Session 方法。
		impl_->session->set_event_handler([this](const ed2k::session::SessionEvent& event) {
			try {
				const auto* task_event = std::get_if<ed2k::session::TaskStateEvent>(&event);
				if (!task_event) {
					// 服务器状态事件：连接后服务器身份/用户数/文件数快照
					if (const auto* srv = std::get_if<ed2k::session::ServerStateEvent>(&event)) {
						nlohmann::json doc;
						doc["connected"] = srv->connected;
						doc["name"] = srv->name;
						doc["high_id"] = srv->high_id;
						doc["users"] = srv->users;
						doc["files"] = srv->files;
						doc["error"] = std::string();
						if (impl_->pubsub) {
							impl_->pubsub->Publish(kEd2kServerState, doc.dump());
						}
					}
					return;
				}
				auto it = impl_->session_to_id.find(task_event->task_id);
				if (it == impl_->session_to_id.end()) {
					return;
				}
				nlohmann::json payload;
				payload["id"] = it->second;
				payload["state"] = TaskStateToString(task_event->state);
				payload["error"] = task_event->error ? task_event->error.message() : std::string();
				if (impl_->pubsub) {
					impl_->pubsub->Publish(kEd2kTaskState, payload.dump());
				}
				// 终态任务不会被引擎自动移出任务表(只有 cancel 会 erase)，若不清理，
				// query_all() 会每秒继续返回它——上层持续收到重复的终态采样并反复写历史。
				// 这里在事件转发之后把任务从引擎移除并清双向映射(历史落库由上层负责，
				// 不删文件，kError 任务重启续传所需的 .part.met 保持原样)。
				// 契约禁止在回调内反调 Session 方法，post 延迟到回调所在协程退出后执行；
				// io_context 单线程 FIFO 保证上面 Publish 派发的订阅回调先于本清理执行。
				if (task_event->state == ed2k::session::TaskState::completed ||
					task_event->state == ed2k::session::TaskState::failed) {
					const std::uint64_t session_id = task_event->task_id;
					boost::asio::post(impl_->runtime.executor(), [this, session_id] {
						if (!impl_->session) {
							return;
						}
						auto sid_it = impl_->session_to_id.find(session_id);
						if (sid_it == impl_->session_to_id.end()) {
							return;
						}
						// 必须先删双向映射再 cancel: cancel 会同步发出 cancelled 事件并再入
						// 本事件回调,若映射仍在,该回声会以 kRemoved 覆盖刚写入的终态历史记录
						// (上层缓存此刻已清,覆盖后的记录元数据为空,真实错误信息被吞)。
						// 映射先删则回声在回调开头查表落空,静默丢弃。
						impl_->id_to_session.erase(sid_it->second);
						impl_->session_to_id.erase(sid_it);
						impl_->session->cancel(session_id, false);
					});
				}
			} catch (...) {
				// 引擎契约：事件回调绝不允许抛出异常
			}
		});

		// 1s 采样定时器：构造后立即 arm 一次；worker 线程启动后 io_context 开始处理挂起的 async_wait。
		// 必须在 worker 线程启动前完成这次 arm（否则和 run() 所在线程存在并发访问同一 timer 的风险）。
		impl_->sample_timer = std::make_unique<boost::asio::steady_timer>(impl_->runtime.context());
		impl_->running.store(true);
		ScheduleSampling();

		// 引擎跑在进程内(不同于 aria2 的外部进程隔离)，run() 中任何逃逸异常都会
		// terminate 整个应用，必须在线程入口兜底
		impl_->worker = std::thread([this] {
			try {
				impl_->runtime.run();
			} catch (const std::exception& e) {
				LOG_ERR("ed2k engine network thread exited with exception: {}", e.what());
			} catch (...) {
				LOG_ERR("ed2k engine network thread exited with unknown exception");
			}
			// IoRuntime 不持 work_guard：run() 返回即代表 io_context 里再无待办工作，
			// 也就代表所有下载协程都已跑完各自的收尾（save_met/discard_met）。
			impl_->run_exited.store(true, std::memory_order_release);
		});
		return true;
	} catch (const std::exception& e) {
		LOG_ERR("Failed to init ed2k engine: {}", e.what());
	} catch (...) {
		LOG_ERR("Failed to init ed2k engine: unknown exception");
	}
	// 初始化中途失败：回滚已构造的部件，保持"未初始化"状态(引擎为可选能力，不影响 aria2)
	impl_->running.store(false);
	impl_->session.reset();
	impl_->sample_timer.reset();
	impl_->pubsub.reset();
	return false;
}

void Ed2kDownloadManager::ShutdownEngine() {
	if (!impl_->running.exchange(false)) {
		return;
	}
	// 修复 Task 2 遗留的 shutdown 顺序 bug：IoRuntime::stop() 直接调用 io_context::stop()，
	// 不会 drain 队列——如果 stop() 在 "post(session->shutdown)" 之外调用，session->shutdown()
	// 这个已排队的任务可能被跳过，导致 Session 未优雅关闭。
	// 故"取消采样定时器 + session->shutdown()"放进同一个网络线程任务里严格按序执行。
	boost::asio::post(impl_->runtime.executor(), [this] {
		if (impl_->sample_timer) {
			// cancel 后挂起的 async_wait 会以 operation_aborted 完成（或因 stop() 直接不再触发），
			// 两种情况下 ScheduleSampling 的完成回调都不会再重排下一次采样。
			impl_->sample_timer->cancel();
		}
		if (impl_->session) {
			impl_->session->shutdown();
		}
		// F6：这里不再紧接着 runtime.stop()。shutdown() 只是给各任务置停标志（并关掉服务器/
		// Kad/监听 socket 促使挂起的 recv 快速出错返回），下载协程还要等自己从在途等待里醒来，
		// 才会跑到收尾处把块级进度 save_met() 落盘、把已完成任务的 .part.met 删掉。若在同一个
		// handler 里立刻 stop()，io_context 当场停摆，这些协程再也没有机会恢复——自上次节流
		// 落盘（每 16 块 ≈ 2.8MiB）以来的进度全部丢失，已下完的任务还会留下 .part.met 垃圾。
	});
	// F6：给协程一段**有上限**的排空时间。IoRuntime 不持 work_guard，所以协程全部收敛后
	// run() 会自然返回（run_exited 置位），此时 join 立即完成——常见情况下这里只等几百毫秒到
	// 1 秒（采样协程的 1s 定时器是实际下限）。超过上限仍未收敛（例如某个 worker 正卡在一次
	// 最长可达 30s 的对端 RPC 等待里）才强制 stop()：退出不能被无界地拖住，此时丢失的进度也
	// 就退化回修复前的水平，不会更差。
	constexpr auto kShutdownDrainTimeout = std::chrono::seconds(3);
	const auto drain_deadline = std::chrono::steady_clock::now() + kShutdownDrainTimeout;
	while (!impl_->run_exited.load(std::memory_order_acquire) &&
		   std::chrono::steady_clock::now() < drain_deadline) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	if (!impl_->run_exited.load(std::memory_order_acquire)) {
		LOG_WARN("ed2k engine did not settle within drain timeout, forcing stop");
		impl_->runtime.stop();  // io_context::stop() 线程安全，可从本线程调用
	}
	if (impl_->worker.joinable()) {
		impl_->worker.join();
	}
	impl_->session.reset();
	impl_->pubsub.reset();
	impl_->sample_timer.reset();
	impl_->id_to_session.clear();
	impl_->session_to_id.clear();
}

bool Ed2kDownloadManager::EngineIsRunning() const {
	return impl_->running.load();
}

void Ed2kDownloadManager::ScheduleSampling() {
	impl_->sample_timer->expires_after(std::chrono::seconds(1));
	impl_->sample_timer->async_wait([this](const boost::system::error_code& ec) {
		// ec 非空：定时器被 cancel（shutdown 中）或对象已析构，直接返回，不再重排
		if (ec) {
			return;
		}
		// 双重保险：shutdown 已置位或 session 已释放时不再采样/重排
		if (!impl_->running.load() || !impl_->session) {
			return;
		}
		try {
			auto snapshots = impl_->session->query_all();
			nlohmann::json arr = nlohmann::json::array();
			for (const auto& snap : snapshots) {
				// 终态转换已由状态事件单独通知,采样只报告未完结任务的进度;
				// 终态任务随后会被事件回调 post 的清理任务移出引擎,这里过滤掉
				// 清理生效前的窗口期,避免上层重复收到终态数据
				if (snap.state == ed2k::session::TaskState::completed ||
					snap.state == ed2k::session::TaskState::failed ||
					snap.state == ed2k::session::TaskState::cancelled) {
					continue;
				}
				auto id_it = impl_->session_to_id.find(snap.id);
				if (id_it == impl_->session_to_id.end()) {
					// 尚未记录映射（理论上 add_download 已同步写入，防御性跳过）
					continue;
				}
				nlohmann::json item;
				item["id"] = id_it->second;
				item["name"] = snap.name;
				item["total"] = snap.total_size;
				item["done"] = snap.bytes_done;
				item["speed"] = snap.speed_bps;
				// sources = 迄今发现的源总数(含已放弃/冷却中的源, 只增不减);
				// active_sources = 此刻真正在连的源数, 即 UI 上"连接数"的口径。
				// 二者都由引擎在源集合/活跃 worker 变化时实时回写(见引擎 download::StatsFn);
				// 此前只有 sources 且它冻结在 dl.run() 启动前那次 GETSOURCES 的数字上。
				item["sources"] = snap.known_sources;
				item["active_sources"] = ActiveSourcesOf(snap);
				item["state"] = TaskStateToString(snap.state);
				// 非终态任务的 error 字段是"当前状态说明"而非失败原因(引擎在等待可用源时会附上
				// 等待原因但不转失败态)。必须随每次采样一起带上:该原因只在变化时通过状态事件推
				// 一次,若采样不带,下一秒的采样就把 UI 上的说明冲掉了,用户又变成对着静默任务发懵。
				item["error"] = snap.error ? snap.error.message() : std::string();
				// 目标文件完整路径，供上层展示“打开文件位置”及重启续传时反推 save_dir
				item["out_path"] = snap.out_path.string();
				arr.push_back(std::move(item));
			}
			if (impl_->pubsub) {
				impl_->pubsub->Publish(kEd2kActiveProgress, arr.dump());
			}
			PublishShareStateLocked();
		} catch (const std::exception&) {
			// 单次采样失败不应中断定时器重排
		}
		ScheduleSampling();
	});
}

std::string Ed2kDownloadManager::AddEd2kTask(const std::string& link, const std::string& save_dir) {
	// 与其它对外方法一致：引擎未初始化或已关闭时直接失败，避免把任务 post 到不再运行的
	// io_context 上——那样会返回一个看似有效但永不入队的 task_id，破坏"失败返回空"契约。
	if (!impl_->running.load()) {
		return {};
	}
	auto parsed = ed2k::parse_link(link);
	if (!parsed) {
		return {};
	}
	auto* file = std::get_if<ed2k::Ed2kFileLink>(&*parsed);
	if (!file) {
		// 只支持文件链接；ServerLink/ServerListLink 不是下载任务
		return {};
	}
	// 任务身份 = md4 hex，与 add_download 返回的自增 session id 是两套 ID，
	// 需在 Impl 维护双向映射供 Pause/Unpause/Remove/采样/事件反查。
	std::string task_id = "ed2k-" + file->hash.to_hex();
	auto link_copy = *file;
	auto dir = std::filesystem::path(save_dir);
	boost::asio::post(impl_->runtime.executor(), [this, task_id, link_copy = std::move(link_copy), dir]() mutable {
		if (!impl_->session) {
			return;
		}
		// 去重:同一 md4 重复添加会产生两个 session id 映射到同一个对外 task_id,
		// 采样将对同一任务每秒发两份数据、RemoveTask 也只能清掉其中一条映射
		if (impl_->id_to_session.count(task_id) > 0) {
			return;
		}
		std::uint64_t session_id = impl_->session->add_download(link_copy, dir);
		impl_->id_to_session[task_id] = session_id;
		impl_->session_to_id[session_id] = task_id;

		// 立即发布一条合成的初始快照(queued 态,元数据取自链接本身),不等首次 1s 采样:
		// 否则任务在首次采样前进入终态(无服务器/端口被占时常见)时,上层只收到仅含
		// {id,state,error} 的状态事件,采样缓存为空导致落库记录 name/size/link 全空——
		// 恢复场景下旧记录已删,等于一次失败就把任务永久弄丢。同时这也让 UI 在添加后
		// 立刻显示带文件名的等待条目。
		try {
			nlohmann::json item;
			item["id"] = task_id;
			item["name"] = link_copy.name;
			item["total"] = static_cast<std::int64_t>(link_copy.size);
			item["done"] = static_cast<std::int64_t>(0);
			item["speed"] = static_cast<std::int64_t>(0);
			item["sources"] = static_cast<std::int64_t>(0);
			item["active_sources"] = static_cast<std::int64_t>(0);
			item["state"] = std::string("queued");
			item["out_path"] = (dir / link_copy.name).string();
			nlohmann::json arr = nlohmann::json::array({std::move(item)});
			if (impl_->pubsub) {
				impl_->pubsub->Publish(kEd2kActiveProgress, arr.dump());
			}
		} catch (const std::exception& e) {
			LOG_ERR("Failed to publish initial ed2k task snapshot: {}", e.what());
		}
	});
	return task_id;
}

bool Ed2kDownloadManager::PauseTask(const std::string& task_id) {
	if (!impl_->running.load()) {
		return false;
	}
	boost::asio::post(impl_->runtime.executor(), [this, task_id] {
		if (!impl_->session) {
			return;
		}
		auto it = impl_->id_to_session.find(task_id);
		if (it != impl_->id_to_session.end()) {
			impl_->session->pause(it->second);
		}
	});
	return true;
}

bool Ed2kDownloadManager::UnpauseTask(const std::string& task_id) {
	if (!impl_->running.load()) {
		return false;
	}
	boost::asio::post(impl_->runtime.executor(), [this, task_id] {
		if (!impl_->session) {
			return;
		}
		auto it = impl_->id_to_session.find(task_id);
		if (it != impl_->id_to_session.end()) {
			impl_->session->resume(it->second);
		}
	});
	return true;
}

bool Ed2kDownloadManager::RemoveTask(const std::string& task_id, bool delete_files) {
	if (!impl_->running.load()) {
		return false;
	}
	boost::asio::post(impl_->runtime.executor(), [this, task_id, delete_files] {
		if (!impl_->session) {
			return;
		}
		auto it = impl_->id_to_session.find(task_id);
		if (it == impl_->id_to_session.end()) {
			return;
		}
		std::uint64_t session_id = it->second;
		impl_->session->cancel(session_id, delete_files);
		impl_->session_to_id.erase(session_id);
		impl_->id_to_session.erase(it);
	});
	return true;
}

void Ed2kDownloadManager::PauseAll() {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this] {
		if (!impl_->session) {
			return;
		}
		for (const auto& [task_id, session_id] : impl_->id_to_session) {
			impl_->session->pause(session_id);
		}
	});
}

void Ed2kDownloadManager::UnpauseAll() {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this] {
		if (!impl_->session) {
			return;
		}
		for (const auto& [task_id, session_id] : impl_->id_to_session) {
			impl_->session->resume(session_id);
		}
	});
}

void Ed2kDownloadManager::Search(const std::string& keyword, int file_type, std::int64_t min_size,
								 int source) {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this, keyword, file_type, min_size, source] {
		// 守卫占用/引擎未就绪时发布失败结果而非静默丢弃：UI 的 searching 状态依赖
		// 结果消息复位，静默丢弃会让用户面对无反馈的"搜索中"直到超时兜底
		if (!impl_->session || impl_->foreground_in_flight) {
			if (impl_->pubsub) {
				impl_->pubsub->Publish(kEd2kSearchResult,
					SearchErrorToJson(!impl_->session ? "engine not ready"
													  : "another server operation is in progress, try again shortly"));
			}
			return;
		}
		impl_->foreground_in_flight = true;
		boost::asio::co_spawn(
			impl_->runtime.executor(),
			[this, keyword, file_type, min_size, source]() -> boost::asio::awaitable<void> {
				try {
					if (source == 1) {
						// Kad 搜索：结果结构不同(KadSearchEntry)，转成与服务器搜索一致的 payload
						auto r = co_await impl_->session->kad_search(keyword);
						if (impl_->pubsub) {
							if (r) {
								nlohmann::json doc;
								doc["ok"] = true;
								doc["error"] = std::string();
								doc["append"] = false;
								nlohmann::json arr = nlohmann::json::array();
								for (const auto& entry : r.value()) {
									nlohmann::json j;
									// KadSearchEntry 的 tag 可能是数值 name_id, 也可能是单字节 string-name
									// (name_id==0 且 name_str 为单字节, 字节值即 tag id) 两种编码。Kad 结果里
									// filename/size 常是 string-name 形式, 若只按 name_id 手动匹配会全部漏掉,
									// 导致 size 恒 0、结果被下面的 continue 全部跳过(表现为"没有结果")。
									// 名称/大小改用引擎 helper(内部 has_name_id 兼容两种形式)。
									const std::string name = ed2k::kad::file_name(entry);
									const std::uint64_t size = ed2k::kad::file_size(entry);
									// 可用源数(tag 0x15)引擎无专用 helper, 内联做同样的双形式匹配
									std::uint64_t sources = 0;
									for (const auto& t : entry.tags) {
										const bool match =
											(t.name_id == 0x15) ||
											(t.name_str.size() == 1 &&
											 static_cast<std::uint8_t>(static_cast<unsigned char>(t.name_str[0])) ==
												 0x15);
										if (match && std::holds_alternative<std::uint64_t>(t.value)) {
											sources = std::get<std::uint64_t>(t.value);
											break;
										}
									}
									if (name.empty() || size == 0) {
										continue;
									}
									j["name"] = name;
									j["size"] = size;
									j["hash"] = entry.answer_id.to_hex();
									j["sources"] = sources;
									j["complete_sources"] = 0;
									arr.push_back(std::move(j));
								}
								doc["items"] = std::move(arr);
								impl_->pubsub->Publish(kEd2kSearchResult, doc.dump());
							} else {
								impl_->pubsub->Publish(kEd2kSearchResult, SearchErrorToJson(r.error().message()));
							}
						}
					} else {
						ed2k::session::SearchFilters filters;
						filters.type = static_cast<ed2k::server::FileType>(file_type);
						filters.min_size = min_size > 0 ? static_cast<std::uint64_t>(min_size) : 0;
						auto r = co_await impl_->session->search(keyword, filters);
						if (impl_->pubsub) {
							impl_->pubsub->Publish(kEd2kSearchResult,
								r ? SearchResultsToJson(r.value(), false) : SearchErrorToJson(r.error().message()));
						}
					}
				} catch (const std::exception& e) {
					LOG_ERR("ed2k search failed: {}", e.what());
					if (impl_->pubsub) {
						impl_->pubsub->Publish(kEd2kSearchResult, SearchErrorToJson(e.what()));
					}
				}
				ReleaseForegroundGuardLocked();
				co_return;
			},
			boost::asio::detached);
	});
}

void Ed2kDownloadManager::SearchMore() {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this] {
		// 同 Search：忙碌/未就绪时发布失败结果，复位 UI 的 searching 状态
		if (!impl_->session || impl_->foreground_in_flight) {
			if (impl_->pubsub) {
				impl_->pubsub->Publish(kEd2kSearchResult,
					SearchErrorToJson(!impl_->session ? "engine not ready"
													  : "another server operation is in progress, try again shortly"));
			}
			return;
		}
		impl_->foreground_in_flight = true;
		boost::asio::co_spawn(impl_->runtime.executor(), [this]() -> boost::asio::awaitable<void> {
			try {
				auto r = co_await impl_->session->search_more();
				if (impl_->pubsub) {
					impl_->pubsub->Publish(kEd2kSearchResult,
						r ? SearchResultsToJson(r.value(), true) : SearchErrorToJson(r.error().message()));
				}
			} catch (const std::exception& e) {
				LOG_ERR("ed2k search_more failed: {}", e.what());
				if (impl_->pubsub) {
					impl_->pubsub->Publish(kEd2kSearchResult, SearchErrorToJson(e.what()));
				}
			}
			ReleaseForegroundGuardLocked();
			co_return;
		}, boost::asio::detached);
	});
}

void Ed2kDownloadManager::ConnectServer(const std::string& ip, std::uint16_t port) {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this, ip, port] {
		if (!impl_->session || impl_->foreground_in_flight) {
			return;
		}
		impl_->foreground_in_flight = true;
		boost::asio::co_spawn(impl_->runtime.executor(), [this, ip, port]() -> boost::asio::awaitable<void> {
			try {
				std::optional<ed2k::app::ServerTarget> target;
				if (!ip.empty()) {
					auto parsed = ed2k::IPv4::from_dotted(ip);
					if (parsed) {
						target = ed2k::app::ServerTarget{parsed.value(), port};
					}
				}
				auto r = co_await impl_->session->connect_server(target);
				nlohmann::json doc;
				if (r) {
					doc["connected"] = true;
					doc["high_id"] = r->high_id;
					doc["error"] = std::string();
					// name/users/files 由 Session 的 ServerStateEvent 补充(见事件转发)
					doc["name"] = std::string();
					doc["users"] = 0;
					doc["files"] = 0;
				} else {
					doc["connected"] = false;
					doc["high_id"] = false;
					doc["name"] = std::string();
					doc["users"] = 0;
					doc["files"] = 0;
					doc["error"] = r.error().message();
				}
				if (impl_->pubsub) {
					impl_->pubsub->Publish(kEd2kServerState, doc.dump());
				}
				// 连接态变化会影响列表 connected 标记，顺带刷新列表
				PublishServerListLocked();
			} catch (const std::exception& e) {
				LOG_ERR("ed2k connect_server failed: {}", e.what());
			}
			ReleaseForegroundGuardLocked();
			co_return;
		}, boost::asio::detached);
	});
}

void Ed2kDownloadManager::DisconnectServer() {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this] {
		// disconnect_server() 是同步调用，不挂起协程，但仍需遵守前台请求串行契约：
		// 若此刻有 search/connect_server 正挂起在同一个 socket 上，
		// 直接断开会破坏对方协程正在使用的连接，因此这里同样检查守卫并丢弃。
		if (!impl_->session || impl_->foreground_in_flight) {
			return;
		}
		impl_->foreground_in_flight = true;
		impl_->session->disconnect_server();
		nlohmann::json doc;
		doc["connected"] = false;
		doc["high_id"] = false;
		doc["error"] = std::string();
		doc["name"] = std::string();
		doc["users"] = 0;
		doc["files"] = 0;
		if (impl_->pubsub) {
			impl_->pubsub->Publish(kEd2kServerState, doc.dump());
		}
		PublishServerListLocked();
		ReleaseForegroundGuardLocked();
	});
}

// 仅网络线程调用：把当前 server_list 序列化发布
void Ed2kDownloadManager::PublishServerListLocked() {
	if (!impl_->session || !impl_->pubsub) {
		return;
	}
	nlohmann::json doc;
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& info : impl_->session->server_list()) {
		nlohmann::json j;
		j["ip"] = info.ip.to_dotted();
		j["port"] = info.port;
		j["name"] = info.name;
		j["connected"] = info.connected;
		j["users"] = info.users;
		j["files"] = info.files;
		j["max_users"] = info.max_users;
		arr.push_back(std::move(j));
	}
	doc["servers"] = std::move(arr);
	impl_->pubsub->Publish(kEd2kServerList, doc.dump());
}

void Ed2kDownloadManager::RequestServerList() {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this] { PublishServerListLocked(); });
}

void Ed2kDownloadManager::AddServer(const std::string& ip, std::uint16_t port, const std::string& name) {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this, ip, port, name] {
		if (!impl_->session) {
			return;
		}
		auto parsed = ed2k::IPv4::from_dotted(ip);
		if (!parsed) {
			return;
		}
		impl_->session->add_server(parsed.value(), port, name);
		PublishServerListLocked();
	});
}

void Ed2kDownloadManager::RemoveServer(const std::string& ip, std::uint16_t port) {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this, ip, port] {
		if (!impl_->session) {
			return;
		}
		auto parsed = ed2k::IPv4::from_dotted(ip);
		if (!parsed) {
			return;
		}
		impl_->session->remove_server(parsed.value(), port);
		PublishServerListLocked();
	});
}

void Ed2kDownloadManager::UpdateServerMet(const std::string& url) {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this, url] {
		// 引擎未就绪也必须发布失败结果：调用方（启动"先更新后连接"链/UI toast）
		// 依赖"每次请求必有一个结果"来推进后续动作，静默丢弃会卡住整条链
		if (!impl_->session) {
			if (impl_->pubsub) {
				nlohmann::json result;
				result["ok"] = false;
				result["error"] = "engine not ready";
				impl_->pubsub->Publish(kEd2kServerMetResult, result.dump());
			}
			return;
		}
		// 重入（上一次更新还在进行中）直接忽略：进行中的更新完成时自会发布结果
		if (impl_->server_met_in_flight) {
			return;
		}
		impl_->server_met_in_flight = true;
		boost::asio::co_spawn(impl_->runtime.executor(), [this, url]() -> boost::asio::awaitable<void> {
			try {
				auto r = co_await impl_->session->update_server_met(url);
				if (!r) {
					LOG_WARN("ed2k update_server_met failed: {}", r.error().message());
				}
				PublishServerListLocked();
				nlohmann::json result;
				result["ok"] = static_cast<bool>(r);
				result["error"] = r ? std::string() : r.error().message();
				if (impl_->pubsub) {
					impl_->pubsub->Publish(kEd2kServerMetResult, result.dump());
				}
			} catch (const std::exception& e) {
				LOG_ERR("ed2k update_server_met exception: {}", e.what());
				if (impl_->pubsub) {
					nlohmann::json result;
					result["ok"] = false;
					result["error"] = e.what();
					impl_->pubsub->Publish(kEd2kServerMetResult, result.dump());
				}
			}
			impl_->server_met_in_flight = false;
			co_return;
		}, boost::asio::detached);
	});
}

// 仅网络线程调用：把当前上传统计与分享文件列表序列化发布
void Ed2kDownloadManager::PublishShareStateLocked() {
	if (!impl_->session || !impl_->pubsub) {
		return;
	}
	const auto stats = impl_->session->upload_stats();
	nlohmann::json doc;
	doc["speed_bps"] = stats.speed_bps;
	doc["queued"] = stats.queued_count;
	doc["active"] = static_cast<std::uint32_t>(stats.active_sessions);
	doc["total_uploaded"] = stats.total_uploaded;
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& f : impl_->session->shared_files()) {
		nlohmann::json j;
		j["name"] = f.name;
		j["path"] = f.path.string();
		j["size"] = f.size;
		j["hash"] = f.hash.to_hex();
		j["uploaded"] = f.uploaded;
		j["requests"] = f.requests;
		arr.push_back(std::move(j));
	}
	doc["files"] = std::move(arr);
	impl_->pubsub->Publish(kEd2kShareState, doc.dump());
}

// 仅网络线程调用：复位前台守卫；若前台守卫占用期间累积了分享目录请求，取出最新一份立即重放。
// 取值在重放前先移出 optional，配合"只保留最新一份"语义可避免无界递归。
void Ed2kDownloadManager::ReleaseForegroundGuardLocked() {
	impl_->foreground_in_flight = false;
	if (impl_->pending_shared_dirs.has_value()) {
		auto paths = std::move(*impl_->pending_shared_dirs);
		impl_->pending_shared_dirs.reset();
		SpawnSetSharedDirsLocked(std::move(paths));
	}
}

// 仅网络线程调用：占用前台守卫并 co_spawn set_shared_dirs 协程；协程结束经
// ReleaseForegroundGuardLocked 释放守卫并重放暂存请求。SetSharedDirs 与守卫释放重放共用本方法。
void Ed2kDownloadManager::SpawnSetSharedDirsLocked(std::vector<std::filesystem::path> paths) {
	impl_->foreground_in_flight = true;
	boost::asio::co_spawn(impl_->runtime.executor(), [this, paths = std::move(paths)]() -> boost::asio::awaitable<void> {
		try {
			auto r = co_await impl_->session->set_shared_dirs(paths);
			if (r) {
				PublishShareStateLocked();
			} else {
				LOG_WARN("ed2k set_shared_dirs failed: {}", r.error().message());
				if (impl_->pubsub) {
					nlohmann::json result;
					result["ok"] = false;
					result["error"] = r.error().message();
					impl_->pubsub->Publish(kEd2kShareOpResult, result.dump());
				}
			}
		} catch (const std::exception& e) {
			LOG_ERR("ed2k set_shared_dirs exception: {}", e.what());
			if (impl_->pubsub) {
				nlohmann::json result;
				result["ok"] = false;
				result["error"] = e.what();
				impl_->pubsub->Publish(kEd2kShareOpResult, result.dump());
			}
		}
		ReleaseForegroundGuardLocked();
		co_return;
	}, boost::asio::detached);
}

void Ed2kDownloadManager::SetSharedDirs(const std::vector<std::string>& dirs) {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this, dirs] {
		if (!impl_->session) {
			return;
		}
		std::vector<std::filesystem::path> paths;
		paths.reserve(dirs.size());
		for (const auto& d : dirs) {
			// 入参为 UTF-8(QString::toStdString 上游)，须经 u8string 构造避免 Windows 按活动
			// 代码页误解码导致中文目录名损坏；data_dir/save_dir 同类问题属既有约定，另行跟进。
			paths.emplace_back(std::u8string(reinterpret_cast<const char8_t*>(d.data()), d.size()));
		}
		// 前台守卫占用期间(如启动时自动连接在途)暂存最新一份请求，守卫释放时重放，避免被静默丢弃
		if (impl_->foreground_in_flight) {
			impl_->pending_shared_dirs = std::move(paths);
			return;
		}
		SpawnSetSharedDirsLocked(std::move(paths));
	});
}

void Ed2kDownloadManager::RequestKadStatus() {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this] {
		if (!impl_->session || !impl_->pubsub) {
			return;
		}
		auto status = impl_->session->kad_status();
		nlohmann::json doc;
		doc["running"] = status.running;
		doc["contacts"] = status.contacts;
		impl_->pubsub->Publish(kEd2kKadStatus, doc.dump());
	});
}

Ed2kDownloadManager::Subscription Ed2kDownloadManager::SubscriptionEd2kMessage(
	const std::string& topic, std::function<void(const std::string&)> handler) {
	return impl_->pubsub->Subscribe(topic, std::move(handler));
}

void Ed2kDownloadManager::UnSubscribeEd2kMessage(Subscription subscription) {
	if (impl_->pubsub) {
		impl_->pubsub->Unsubscribe(std::move(subscription));
	}
}

}  // namespace engine
}  // namespace gdl
