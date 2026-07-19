#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "Ed2kEngine_export.h"
#include "publish_subscribe_system.h"
#include "singleton.hpp"

namespace gdl {
namespace engine {

// ed2k 下载管理器：单例，内部持有独立 worker 线程运行 ed2k::session::Session。
// 所有对外接口线程安全（对 Session 的实际访问会 post 到网络线程执行）。
class Ed2kEngine_API Ed2kDownloadManager : public Singleton<Ed2kDownloadManager> {
	SINGLETON_DECLARE(Ed2kDownloadManager)
   public:
	using Subscription = std::shared_ptr<PubSubSystem<std::string>::Subscription>;

	struct Ed2kEngineConfig {
		std::string nickname = "GDownload";
		std::uint16_t tcp_port = 4662;
		std::uint16_t udp_port = 4672;
		std::string data_dir;  // <app数据目录>/ed2k
		std::size_t max_concurrent_tasks = 5;
		bool enable_kad = false;
	};

	~Ed2kDownloadManager();

	// 初始化引擎：构造 Session 并启动网络线程；已运行时直接返回 true（幂等）
	bool InitEd2kEngine(const Ed2kEngineConfig& config);
	// 关闭引擎：网络线程上优雅关闭 Session，再停止 IoRuntime 并 join 线程；幂等
	void ShutdownEngine();
	bool EngineIsRunning() const;

	// Task 4 实现：返回 "ed2k-<hash>" 任务 ID；link 为完整 ed2k:// 字符串
	std::string AddEd2kTask(const std::string& link, const std::string& save_dir);
	bool PauseTask(const std::string& task_id);
	bool UnpauseTask(const std::string& task_id);
	bool RemoveTask(const std::string& task_id, bool delete_files);
	void PauseAll();
	void UnpauseAll();

	Subscription SubscriptionEd2kMessage(const std::string& topic,
										 std::function<void(const std::string&)> handler);
	void UnSubscribeEd2kMessage(Subscription subscription);

   private:
	explicit Ed2kDownloadManager();
	// 安排下一次 1s 采样（内部递归重排，网络线程执行）
	void ScheduleSampling();
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

}  // namespace engine
}  // namespace gdl
