#pragma once
#include <atomic>
#include "GDLCore/globalTypes.h"
#include "GDLCore/singleton.hpp"
#include "export.h"
namespace gdl {
	namespace engine {
		// 这里主要用来管理aria2c 的各种功能类 外部封装
		class Engine_API Aria2cDownloadManager : public Singleton<Aria2cDownloadManager> {
			SINGLETON_DECLARE(Aria2cDownloadManager)
		   public:
			~Aria2cDownloadManager();
			bool InitAria2cEngine();
			void UninitAria2cEngine();
			bool EngineIsRuning() const { return engine_is_runing_; }

		   private:
			explicit Aria2cDownloadManager(const String_View& aria2c_path);

		   private:
			String aria2c_path_;
			std::atomic_bool engine_is_runing_{false};
		};
	}  // namespace engine

}  // namespace gdl
