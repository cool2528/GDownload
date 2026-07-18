#pragma once
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "PluginManager_export.h"

namespace gdl {
	namespace plugin {

		// 插件配置值：string/bool/number 三态，与 manifest settings type 对应
		struct PluginManager_API ConfigValue {
			enum class Type { String, Bool, Number };
			Type type{Type::String};
			std::string string_value;
			bool bool_value{false};
			double number_value{0};

			static ConfigValue FromString(std::string v) {
				ConfigValue value;
				value.type		   = Type::String;
				value.string_value = std::move(v);
				return value;
			}
			static ConfigValue FromBool(bool v) {
				ConfigValue value;
				value.type		 = Type::Bool;
				value.bool_value = v;
				return value;
			}
			static ConfigValue FromNumber(double v) {
				ConfigValue value;
				value.type		   = Type::Number;
				value.number_value = v;
				return value;
			}
		};

		// 按插件名隔离的用户配置存储：<data_dir>/plugin_configs.json
		// 无状态设计：每次读取都加载文件，写入用临时文件+rename 原子落盘；
		// 多实例共享同一文件也安全（写入方仅 UI 线程，实际单写者）
		class PluginManager_API PluginConfigStore {
		   public:
			explicit PluginConfigStore(std::filesystem::path data_dir);

			// 读取某插件全部配置；文件缺失/损坏按空处理
			std::map<std::string, ConfigValue> GetConfig(const std::string& plugin_name) const;
			// 整体替换某插件配置并落盘
			bool SetConfig(const std::string& plugin_name, const std::map<std::string, ConfigValue>& values);
			// 删除某插件全部配置
			bool RemoveConfig(const std::string& plugin_name);
			// 读取单个键
			std::optional<ConfigValue> GetValue(const std::string& plugin_name, const std::string& key) const;

		   private:
			std::filesystem::path file_path_;
			mutable std::mutex mutex_;
		};

	}  // namespace plugin
}  // namespace gdl
