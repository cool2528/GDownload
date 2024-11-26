#pragma once
#include <atomic>
#include <boost/asio.hpp>
#include "Engine_export.h"
#include "globalTypes.h"
#include "singleton.hpp"
#include "timer/asio_async_timer.h"
#include "timer/daily_task_timer.h"
namespace gdl {
	namespace engine {
		enum class IP_VERSION : int { V4 = 4, V6 = 6 };
		class Engine_API Aria2cDownloadManager : public Singleton<Aria2cDownloadManager> {
			SINGLETON_DECLARE(Aria2cDownloadManager)
		   public:
			~Aria2cDownloadManager();
			bool InitAria2cEngine(const String_View& aria2c_path);
			void UninitAria2cEngine();
			bool EngineIsRuning() const { return engine_is_runing_; }

		   private:
			explicit Aria2cDownloadManager();
			std::vector<String_View> InitAria2cSettingsArgs();
			String GetDhtPath(IP_VERSION protocol);
			void UpdateAria2cTasks();

		   private:
			String aria2c_path_;
			std::atomic_bool engine_is_runing_{false};
			boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
			boost::asio::io_context io_context_;
			DailyTaskTimer daily_task_timer_;
			AsyncTimer update_aria2c_tasks_timer_;
			std::thread worker_;
		};
	}  // namespace engine

}  // namespace gdl
