#include "ed2k_download_manager.h"

#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <map>
#include <thread>
#include <variant>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>

#include <nlohmann/json.hpp>

#include <ed2k/link/ed2k_link.hpp>
#include <ed2k/net/runtime.hpp>
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
};

Ed2kDownloadManager::Ed2kDownloadManager() : impl_(std::make_unique<Impl>()) {}

Ed2kDownloadManager::~Ed2kDownloadManager() {
	ShutdownEngine();
}

bool Ed2kDownloadManager::InitEd2kEngine(const Ed2kEngineConfig& config) {
	if (impl_->running.load()) {
		return true;
	}
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
	impl_->session = std::make_unique<ed2k::session::Session>(impl_->runtime, scfg);

	// 任务状态突变事件：反查 session_to_id 转为对外 task_id，转发到 PubSub。
	// 契约要求 handler 不得抛出异常，也不得在回调内反调 Session 方法。
	impl_->session->set_event_handler([this](const ed2k::session::SessionEvent& event) {
		try {
			const auto* task_event = std::get_if<ed2k::session::TaskStateEvent>(&event);
			if (!task_event) {
				// ServerStateEvent 等其它事件暂不在 Task 4 范围内处理
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
		} catch (...) {
			// 引擎契约：事件回调绝不允许抛出异常
		}
	});

	// 1s 采样定时器：构造后立即 arm 一次；worker 线程启动后 io_context 开始处理挂起的 async_wait。
	// 必须在 worker 线程启动前完成这次 arm（否则和 run() 所在线程存在并发访问同一 timer 的风险）。
	impl_->sample_timer = std::make_unique<boost::asio::steady_timer>(impl_->runtime.context());
	impl_->running.store(true);
	ScheduleSampling();

	impl_->worker = std::thread([this] { impl_->runtime.run(); });
	return true;
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
		std::uint64_t session_id = impl_->session->add_download(link_copy, dir);
		impl_->id_to_session[task_id] = session_id;
		impl_->session_to_id[session_id] = task_id;
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
