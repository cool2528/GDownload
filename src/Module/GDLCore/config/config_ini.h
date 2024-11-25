#pragma once
#include <boost/property_tree/ini_parser.hpp>
#include <shared_mutex>
#include "globalTypes.h"
#include "singleton.hpp"
namespace gdl {
	namespace config {
		class ApplicationConfig : public Singleton<ApplicationConfig> {
			SINGLETON_DECLARE(ApplicationConfig)
		   public:
			~ApplicationConfig();
			template <typename T>
			void Put(const std::string& key, const T& value) {
				std::unique_lock lock(mutex_);
				ptree_root_.put<T>(key, value);
				lock.unlock();
				Save();
			}
			template <typename Type>
			Type Get(const std::string& key) {
				std::shared_lock lock(mutex_);
				return ptree_root_.get<Type>(key);
			}

		   private:
			explicit ApplicationConfig();
			bool Load();
			bool Save();

		   private:
			boost::property_tree::ptree ptree_root_;
			String config_file_path_;
			std::shared_mutex mutex_;
		};
	}  // namespace config
}  // namespace gdl
