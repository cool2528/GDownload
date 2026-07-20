#include "ed2k_download_manager.h"

#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <map>
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

#include <ed2k/link/ed2k_link.hpp>
#include <ed2k/net/runtime.hpp>
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
	Ed2kEngineConfig config;

	// 对外任务 ID("ed2k-<md4>") 与引擎内部 session task id(uint64) 的双向映射
	std::map<std::string, std::uint64_t> id_to_session;
	std::map<std::uint64_t, std::string> session_to_id;

	// 1s 采样定时器，仅在网络线程上 arm/cancel
	std::unique_ptr<boost::asio::steady_timer> sample_timer;

	// 搜索/连接前台请求 in-flight 守卫：Session 同连接前台请求必须串行(引擎契约)，
	// 进行中忽略新请求；仅在网络线程读写，无需加锁
	bool search_in_flight = false;
	bool connect_in_flight = false;
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
						impl_->session->cancel(session_id, false);
						impl_->id_to_session.erase(sid_it->second);
						impl_->session_to_id.erase(sid_it);
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
	// 现在把"取消采样定时器 + session->shutdown() + runtime.stop()"放进同一个网络线程任务，
	// 保证严格按此顺序执行，且 stop() 一定在 shutdown() 之后才被调用。
	boost::asio::post(impl_->runtime.executor(), [this] {
		if (impl_->sample_timer) {
			// cancel 后挂起的 async_wait 会以 operation_aborted 完成（或因 stop() 直接不再触发），
			// 两种情况下 ScheduleSampling 的完成回调都不会再重排下一次采样。
			impl_->sample_timer->cancel();
		}
		if (impl_->session) {
			impl_->session->shutdown();
		}
		impl_->runtime.stop();
	});
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
				item["sources"] = snap.known_sources;
				item["state"] = TaskStateToString(snap.state);
				// 目标文件完整路径，供上层展示“打开文件位置”及重启续传时反推 save_dir
				item["out_path"] = snap.out_path.string();
				arr.push_back(std::move(item));
			}
			if (impl_->pubsub) {
				impl_->pubsub->Publish(kEd2kActiveProgress, arr.dump());
			}
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
		if (!impl_->session || impl_->search_in_flight) {
			return;
		}
		impl_->search_in_flight = true;
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
									// KadSearchEntry 的名称/大小从 tags 提取(kad tag id: name=0x01 size=0x02 sources=0x15)
									std::string name;
									std::uint64_t size = 0;
									std::uint64_t sources = 0;
									for (const auto& t : entry.tags) {
										if (t.name_id == 0x01 && std::holds_alternative<std::string>(t.value)) {
											name = std::get<std::string>(t.value);
										} else if (t.name_id == 0x02 && std::holds_alternative<std::uint64_t>(t.value)) {
											size = std::get<std::uint64_t>(t.value);
										} else if (t.name_id == 0x15 && std::holds_alternative<std::uint64_t>(t.value)) {
											sources = std::get<std::uint64_t>(t.value);
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
				impl_->search_in_flight = false;
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
		if (!impl_->session || impl_->search_in_flight) {
			return;
		}
		impl_->search_in_flight = true;
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
			impl_->search_in_flight = false;
			co_return;
		}, boost::asio::detached);
	});
}

void Ed2kDownloadManager::ConnectServer(const std::string& ip, std::uint16_t port) {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this, ip, port] {
		if (!impl_->session || impl_->connect_in_flight) {
			return;
		}
		impl_->connect_in_flight = true;
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
			impl_->connect_in_flight = false;
			co_return;
		}, boost::asio::detached);
	});
}

void Ed2kDownloadManager::DisconnectServer() {
	if (!impl_->running.load()) {
		return;
	}
	boost::asio::post(impl_->runtime.executor(), [this] {
		if (!impl_->session) {
			return;
		}
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
		if (!impl_->session) {
			return;
		}
		boost::asio::co_spawn(impl_->runtime.executor(), [this, url]() -> boost::asio::awaitable<void> {
			try {
				auto r = co_await impl_->session->update_server_met(url);
				if (!r) {
					LOG_WARN("ed2k update_server_met failed: {}", r.error().message());
				}
				PublishServerListLocked();
			} catch (const std::exception& e) {
				LOG_ERR("ed2k update_server_met exception: {}", e.what());
			}
			co_return;
		}, boost::asio::detached);
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
