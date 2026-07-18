#include "plugin_config_store.h"

#include <fstream>
#include <nlohmann/json.hpp>

namespace gdl {
	namespace plugin {

		namespace {
			// 读取整个配置文件；缺失/损坏返回空对象
			nlohmann::json LoadFile(const std::filesystem::path& path) {
				std::ifstream file(path);
				if (!file.is_open()) {
					return nlohmann::json::object();
				}
				try {
					nlohmann::json json;
					file >> json;
					return json.is_object() ? json : nlohmann::json::object();
				} catch (const nlohmann::json::exception&) {
					return nlohmann::json::object();
				}
			}

			// 原子写入：临时文件 + rename
			bool SaveFile(const std::filesystem::path& path, const nlohmann::json& json) {
				std::error_code ec;
				std::filesystem::create_directories(path.parent_path(), ec);
				auto tmp_path = path;
				tmp_path += ".tmp";
				{
					std::ofstream file(tmp_path, std::ios::trunc);
					if (!file.is_open()) {
						return false;
					}
					file << json.dump();
				}
				std::filesystem::rename(tmp_path, path, ec);
				return !ec;
			}

			nlohmann::json ValueToJson(const ConfigValue& value) {
				switch (value.type) {
					case ConfigValue::Type::Bool:
						return value.bool_value;
					case ConfigValue::Type::Number:
						return value.number_value;
					case ConfigValue::Type::String:
					default:
						return value.string_value;
				}
			}

			std::optional<ConfigValue> JsonToValue(const nlohmann::json& json) {
				if (json.is_boolean()) {
					return ConfigValue::FromBool(json.get<bool>());
				}
				if (json.is_number()) {
					return ConfigValue::FromNumber(json.get<double>());
				}
				if (json.is_string()) {
					return ConfigValue::FromString(json.get<std::string>());
				}
				return std::nullopt;
			}
		}  // namespace

		PluginConfigStore::PluginConfigStore(std::filesystem::path data_dir)
			: file_path_(std::move(data_dir) / "plugin_configs.json") {}

		std::map<std::string, ConfigValue> PluginConfigStore::GetConfig(const std::string& plugin_name) const {
			std::lock_guard<std::mutex> lock(mutex_);
			std::map<std::string, ConfigValue> result;
			auto json = LoadFile(file_path_);
			auto it	  = json.find(plugin_name);
			if (it == json.end() || !it->is_object()) {
				return result;
			}
			for (auto entry = it->begin(); entry != it->end(); ++entry) {
				auto value = JsonToValue(entry.value());
				if (value) {
					result[entry.key()] = std::move(*value);
				}
			}
			return result;
		}

		bool PluginConfigStore::SetConfig(const std::string& plugin_name,
										  const std::map<std::string, ConfigValue>& values) {
			std::lock_guard<std::mutex> lock(mutex_);
			auto json = LoadFile(file_path_);
			nlohmann::json plugin_obj = nlohmann::json::object();
			for (const auto& [key, value] : values) {
				plugin_obj[key] = ValueToJson(value);
			}
			json[plugin_name] = std::move(plugin_obj);
			return SaveFile(file_path_, json);
		}

		bool PluginConfigStore::RemoveConfig(const std::string& plugin_name) {
			std::lock_guard<std::mutex> lock(mutex_);
			auto json = LoadFile(file_path_);
			json.erase(plugin_name);
			return SaveFile(file_path_, json);
		}

		std::optional<ConfigValue> PluginConfigStore::GetValue(const std::string& plugin_name,
															   const std::string& key) const {
			auto config = GetConfig(plugin_name);
			auto it		= config.find(key);
			if (it == config.end()) {
				return std::nullopt;
			}
			return it->second;
		}
	}  // namespace plugin
}  // namespace gdl
