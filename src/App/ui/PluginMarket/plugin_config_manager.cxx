#include "plugin_config_manager.h"

#include <nlohmann/json.hpp>

#include "PluginManager/plugin_manager.h"
#include "Settings/settings_manager.h"
#include "language/language_manager.h"

namespace gdl {
	namespace ui {
		namespace market {

			namespace {
				// default_json（JSON 文本）转 QVariant；空/非法返回无效 QVariant
				QVariant DefaultJsonToVariant(const std::string& default_json) {
					if (default_json.empty()) {
						return {};
					}
					try {
						auto json = nlohmann::json::parse(default_json);
						if (json.is_boolean()) {
							return json.get<bool>();
						}
						if (json.is_number()) {
							return json.get<double>();
						}
						if (json.is_string()) {
							return QString::fromStdString(json.get<std::string>());
						}
					} catch (const nlohmann::json::exception&) {
					}
					return {};
				}

				QVariant ConfigValueToVariant(const plugin::ConfigValue& value) {
					switch (value.type) {
						case plugin::ConfigValue::Type::Bool:
							return value.bool_value;
						case plugin::ConfigValue::Type::Number:
							return value.number_value;
						case plugin::ConfigValue::Type::String:
						default:
							return QString::fromStdString(value.string_value);
					}
				}
			}  // namespace

			PluginConfigManager::PluginConfigManager(QObject* parent) : QObject(parent) {}
			PluginConfigManager::~PluginConfigManager() {}

			void PluginConfigManager::Initialize(const QString& data_dir) {
				store_ = std::make_unique<plugin::PluginConfigStore>(data_dir.toStdString());
				// 旧版百度 Cookie 一次性迁移：仅当 store 中还没有 baidu-netdisk 配置时写入；
				// 旧设置项保留不清空，保证可回退
				const auto legacy = settings::Settings::Instance().GetBaiduPanCookies();
				if (!legacy.isEmpty() && store_->GetConfig("baidu-netdisk").empty()) {
					std::map<std::string, plugin::ConfigValue> values;
					values["cookie"] = plugin::ConfigValue::FromString(legacy.toStdString());
					store_->SetConfig("baidu-netdisk", values);
				}
			}

			std::string PluginConfigManager::CurrentLocale() const {
				return language::LanguageManager::Instance().GetCurrentLanguage().toStdString();
			}

			QVariantList PluginConfigManager::schema(const QString& name) const {
				QVariantList result;
				auto manifest = plugin::DownloadPluginManager::Instance().GetManifestByName(name.toStdString());
				if (!manifest) {
					return result;
				}
				const auto locale = CurrentLocale();
				for (const auto& field : manifest->settings) {
					QVariantMap item;
					item["key"]		 = QString::fromStdString(field.key);
					item["type"]	 = QString::fromStdString(field.type);
					item["label"]	 = QString::fromStdString(field.LocalizedLabel(locale));
					item["hint"]	 = QString::fromStdString(field.LocalizedHint(locale));
					item["required"] = field.required;
					item["role"]	 = QString::fromStdString(field.role);
					QStringList options;
					for (const auto& option : field.options) {
						options.push_back(QString::fromStdString(option));
					}
					item["options"]		 = options;
					item["defaultValue"] = DefaultJsonToVariant(field.default_json);
					result.push_back(item);
				}
				return result;
			}

			QVariantMap PluginConfigManager::values(const QString& name) const {
				QVariantMap result;
				if (!store_) {
					return result;
				}
				auto manifest = plugin::DownloadPluginManager::Instance().GetManifestByName(name.toStdString());
				// 先回填 schema default
				if (manifest) {
					for (const auto& field : manifest->settings) {
						auto def = DefaultJsonToVariant(field.default_json);
						if (def.isValid()) {
							result[QString::fromStdString(field.key)] = def;
						}
					}
				}
				// 用户已存值覆盖
				for (const auto& [key, value] : store_->GetConfig(name.toStdString())) {
					result[QString::fromStdString(key)] = ConfigValueToVariant(value);
				}
				return result;
			}

			void PluginConfigManager::save(const QString& name, const QVariantMap& new_values) {
				if (!store_) {
					return;
				}
				auto manifest = plugin::DownloadPluginManager::Instance().GetManifestByName(name.toStdString());
				std::map<std::string, plugin::ConfigValue> converted;
				for (auto it = new_values.begin(); it != new_values.end(); ++it) {
					const auto key = it.key().toStdString();
					// 按 schema type 决定存储类型；schema 未知的键按字符串兜底
					std::string field_type = "text";
					if (manifest) {
						for (const auto& field : manifest->settings) {
							if (field.key == key) {
								field_type = field.type;
								break;
							}
						}
					}
					if (field_type == "bool") {
						converted[key] = plugin::ConfigValue::FromBool(it.value().toBool());
					}
					else if (field_type == "number") {
						converted[key] = plugin::ConfigValue::FromNumber(it.value().toDouble());
					}
					else {
						converted[key] = plugin::ConfigValue::FromString(it.value().toString().toStdString());
					}
				}
				store_->SetConfig(name.toStdString(), converted);
				Q_EMIT configChanged(name);
			}

			void PluginConfigManager::clear(const QString& name) {
				if (!store_) {
					return;
				}
				store_->RemoveConfig(name.toStdString());
				Q_EMIT configChanged(name);
			}

			bool PluginConfigManager::configured(const QString& name) const {
				auto manifest = plugin::DownloadPluginManager::Instance().GetManifestByName(name.toStdString());
				if (!manifest || manifest->settings.empty()) {
					return true;  // 无 schema 视为无需配置
				}
				const auto current = values(name);
				for (const auto& field : manifest->settings) {
					if (!field.required) {
						continue;
					}
					const auto key = QString::fromStdString(field.key);
					if (!current.contains(key)) {
						return false;
					}
					// 文本类字段空串视为未填
					const bool text_like = field.type == "text" || field.type == "password"
										   || field.type == "textarea" || field.type == "select";
					if (text_like && current.value(key).toString().isEmpty()) {
						return false;
					}
				}
				return true;
			}

			bool PluginConfigManager::hasSchema(const QString& name) const {
				auto manifest = plugin::DownloadPluginManager::Instance().GetManifestByName(name.toStdString());
				return manifest.has_value() && !manifest->settings.empty();
			}

			QVariantMap PluginConfigManager::pluginInfo(const QString& name) const {
				QVariantMap info;
				info["name"] = name;
				auto manifest = plugin::DownloadPluginManager::Instance().GetManifestByName(name.toStdString());
				if (manifest) {
					info["displayName"] = QString::fromStdString(manifest->LocalizedDisplayName(CurrentLocale()));
					info["needsConfig"] = !manifest->settings.empty();
				}
				else {
					info["displayName"] = name;
					info["needsConfig"] = false;
				}
				info["configured"] = configured(name);
				return info;
			}

			QString PluginConfigManager::TokenFor(const QString& name) const {
				if (!store_) {
					return {};
				}
				auto manifest = plugin::DownloadPluginManager::Instance().GetManifestByName(name.toStdString());
				if (!manifest) {
					return {};
				}
				const auto* token_field = manifest->TokenField();
				if (!token_field) {
					return {};
				}
				auto value = store_->GetValue(name.toStdString(), token_field->key);
				if (!value) {
					return {};
				}
				return QString::fromStdString(value->string_value);
			}

		}  // namespace market
	}  // namespace ui
}  // namespace gdl
