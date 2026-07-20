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

	// ---- Phase 2: 搜索与服务器管理（全部线程安全，post 到网络线程执行，结果经 PubSub 发布）----
	// 发起搜索。file_type 对应 ed2k::server::FileType 枚举值(0=Any)；source 0=服务器 1=Kad。
	// 结果发布到 kEd2kSearchResult；搜索进行中重复调用会被忽略(引擎前台请求必须串行)。
	void Search(const std::string& keyword, int file_type, std::int64_t min_size, int source);
	// 取回上一次服务器搜索的下一批结果（append=true 发布）
	void SearchMore();
	// 连接服务器；ip 为空串时由引擎自动选择。结果/状态发布到 kEd2kServerState。
	void ConnectServer(const std::string& ip, std::uint16_t port);
	void DisconnectServer();
	// 请求服务器列表快照，发布到 kEd2kServerList
	void RequestServerList();
	void AddServer(const std::string& ip, std::uint16_t port, const std::string& name);
	void RemoveServer(const std::string& ip, std::uint16_t port);
	// 从 URL 更新 server.met，完成后自动发布最新列表
	void UpdateServerMet(const std::string& url);
	// 请求 Kad 状态快照，发布到 kEd2kKadStatus
	void RequestKadStatus();

	Subscription SubscriptionEd2kMessage(const std::string& topic,
										 std::function<void(const std::string&)> handler);
	void UnSubscribeEd2kMessage(Subscription subscription);

   private:
	explicit Ed2kDownloadManager();
	// 安排下一次 1s 采样（内部递归重排，网络线程执行）
	void ScheduleSampling();
	// 仅在网络线程调用：序列化当前服务器列表并发布 kEd2kServerList
	void PublishServerListLocked();
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

}  // namespace engine
}  // namespace gdl
