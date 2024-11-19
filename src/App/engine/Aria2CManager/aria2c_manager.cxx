#include "aria2c_manager.h"
namespace gdl {
	namespace engine {

		namespace detail {}
		Aria2cDownloadManager::Aria2cDownloadManager(const String_View& aria2c_path) : aria2c_path_(aria2c_path) {}

		Aria2cDownloadManager::~Aria2cDownloadManager() {}

		bool Aria2cDownloadManager::InitAria2cEngine() {
			// todo 初始化aria2c
			return false;
		}

		void Aria2cDownloadManager::UninitAria2cEngine() {
			// todo 卸载aria2c
		}

	}  // namespace engine

}  // namespace gdl
