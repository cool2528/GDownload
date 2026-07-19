#include "ed2k_download_manager.h"

#include <atomic>
#include <filesystem>
#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <ed2k/net/runtime.hpp>
#include <ed2k/session/session.hpp>

#include "ed2k_engine_def.h"

namespace gdl {
namespace engine {

// 内部实现：网络线程运行 ed2k::net::IoRuntime，Session 在其上构造/析构。
struct Ed2kDownloadManager::Impl {
	ed2k::net::IoRuntime runtime;
	std::unique_ptr<ed2k::session::Session> session;
	std::unique_ptr<PubSubSystem<std::string>> pubsub;
	std::thread worker;  // 跑 runtime.run() 的网络线程
	std::atomic_bool running{false};
	Ed2kEngineConfig config;
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

	impl_->running.store(true);
	impl_->worker = std::thread([this] { impl_->runtime.run(); });
	return true;
}

void Ed2kDownloadManager::ShutdownEngine() {
	if (!impl_->running.exchange(false)) {
		return;
	}
	// 在网络线程上关闭 Session（优雅收尾/落盘），再停止 IoRuntime
	boost::asio::post(impl_->runtime.executor(), [this] {
		if (impl_->session) {
			impl_->session->shutdown();
		}
	});
	impl_->runtime.stop();
	if (impl_->worker.joinable()) {
		impl_->worker.join();
	}
	impl_->session.reset();
	impl_->pubsub.reset();
}

bool Ed2kDownloadManager::EngineIsRunning() const {
	return impl_->running.load();
}

// --- Task 4 实现，先桩 ---
std::string Ed2kDownloadManager::AddEd2kTask(const std::string& /*link*/, const std::string& /*save_dir*/) {
	return {};
}

bool Ed2kDownloadManager::PauseTask(const std::string& /*task_id*/) {
	return false;
}

bool Ed2kDownloadManager::UnpauseTask(const std::string& /*task_id*/) {
	return false;
}

bool Ed2kDownloadManager::RemoveTask(const std::string& /*task_id*/, bool /*delete_files*/) {
	return false;
}

void Ed2kDownloadManager::PauseAll() {}

void Ed2kDownloadManager::UnpauseAll() {}

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
