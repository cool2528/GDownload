#pragma once
#include <atomic>
#include "Engine_export.h"
#include "globalTypes.h"
#include "singleton.hpp"
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

		   private:
			String aria2c_path_;
			std::atomic_bool engine_is_runing_{false};
		};
	}  // namespace engine

}  // namespace gdl
