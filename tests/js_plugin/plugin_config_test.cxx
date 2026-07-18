// 插件配置系统单元测试：manifest settings schema / PluginConfigStore / gdl.config / GetManifestByName
// 无网络依赖；临时目录建在系统 temp 下，进程退出前清理
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "JsPluginRuntime/plugin_manifest.h"
#include "plugin_config_store.h"
#include "plugin_manager.h"

namespace fs = std::filesystem;

static int g_failures = 0;
#define CHECK(cond)                                                        \
	do {                                                                   \
		if (!(cond)) {                                                     \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
			++g_failures;                                                  \
		}                                                                  \
	} while (0)

// 写一个带指定 settings 片段的 manifest.json 到 dir 并解析
static std::optional<gdl::plugin::js::PluginManifest> LoadWithSettings(const fs::path& dir,
																	   const std::string& settings_json,
																	   std::string& error) {
	fs::create_directories(dir);
	std::ofstream f(dir / "manifest.json", std::ios::trunc);
	f << R"({
		"manifest_version": 1,
		"name": "test-plugin",
		"version": "1.0.0",
		"entry": "main.js",
		"type": "netdisk",
		"url_patterns": ["*://example.com*"],
		"permissions": { "http": ["example.com"] })";
	if (!settings_json.empty()) {
		f << ",\n\"settings\": " << settings_json;
	}
	f << "\n}";
	f.close();
	return gdl::plugin::js::LoadManifest(dir, error);
}

static void TestManifestSettings(const fs::path& root) {
	std::string error;

	// 1. 无 settings 字段：向后兼容，正常加载且 settings 为空
	auto m = LoadWithSettings(root / "no-settings", "", error);
	CHECK(m.has_value());
	CHECK(m->settings.empty());
	CHECK(m->TokenField() == nullptr);

	// 2. 合法 settings：字段全解析 + 本地化回退
	auto ok = LoadWithSettings(root / "ok", R"([
		{ "key": "cookie", "type": "textarea", "required": true, "role": "token",
		  "label": "Cookie", "hint": "Paste cookie",
		  "locales": { "zh_CN": { "label": "登录 Cookie", "hint": "粘贴 Cookie" } } },
		{ "key": "use_cdn", "type": "bool", "default": true, "label": "Use CDN" },
		{ "key": "quality", "type": "select", "default": "high",
		  "options": ["high", "low"], "label": "Quality" }
	])", error);
	CHECK(ok.has_value());
	CHECK(ok->settings.size() == 3);
	CHECK(ok->settings[0].key == "cookie");
	CHECK(ok->settings[0].required);
	CHECK(ok->settings[0].role == "token");
	CHECK(ok->settings[0].LocalizedLabel("zh_CN") == "登录 Cookie");
	CHECK(ok->settings[0].LocalizedLabel("ja_JP") == "Cookie");   // 缺失回退默认
	CHECK(ok->settings[0].LocalizedHint("zh_CN") == "粘贴 Cookie");
	CHECK(ok->settings[1].default_json == "true");
	CHECK(ok->settings[2].options.size() == 2);
	CHECK(ok->TokenField() != nullptr);
	CHECK(ok->TokenField()->key == "cookie");

	// 3. 非法：key 含大写
	CHECK(!LoadWithSettings(root / "bad-key", R"([{ "key": "Bad", "type": "text", "label": "x" }])", error));
	// 4. 非法：key 重复
	CHECK(!LoadWithSettings(root / "dup-key", R"([
		{ "key": "a", "type": "text", "label": "x" },
		{ "key": "a", "type": "bool", "label": "y" }
	])", error));
	// 5. 非法：未知 type
	CHECK(!LoadWithSettings(root / "bad-type", R"([{ "key": "a", "type": "color", "label": "x" }])", error));
	// 6. 非法：select 无 options
	CHECK(!LoadWithSettings(root / "no-opts", R"([{ "key": "a", "type": "select", "label": "x" }])", error));
	// 7. 非法：两个 token role
	CHECK(!LoadWithSettings(root / "two-token", R"([
		{ "key": "a", "type": "text", "role": "token", "label": "x" },
		{ "key": "b", "type": "text", "role": "token", "label": "y" }
	])", error));
	// 8. 非法：token role 用在 bool 上
	CHECK(!LoadWithSettings(root / "bool-token", R"([{ "key": "a", "type": "bool", "role": "token", "label": "x" }])", error));
	// 9. 非法：role 不是 token
	CHECK(!LoadWithSettings(root / "bad-role", R"([{ "key": "a", "type": "text", "role": "admin", "label": "x" }])", error));
	// 10. 非法：缺 label
	CHECK(!LoadWithSettings(root / "no-label", R"([{ "key": "a", "type": "text" }])", error));
	// 11. 非法：required 类型错误（字符串而非 bool）不得抛异常，应拒绝加载
	CHECK(!LoadWithSettings(root / "bad-required", R"([{ "key": "a", "type": "text", "label": "x", "required": "yes" }])", error));
	// 12. 非法：options 元素类型错误（数字而非字符串）不得抛异常，应拒绝加载
	CHECK(!LoadWithSettings(root / "bad-options", R"([{ "key": "a", "type": "select", "label": "x", "options": [1, 2] }])", error));
}

static void TestConfigStore(const fs::path& dir) {
	fs::create_directories(dir);
	using gdl::plugin::ConfigValue;
	gdl::plugin::PluginConfigStore store(dir);

	// 1. 空 store：读为空、GetValue 为 nullopt
	CHECK(store.GetConfig("quark").empty());
	CHECK(!store.GetValue("quark", "cookie").has_value());

	// 2. 写入并读回三种类型
	std::map<std::string, ConfigValue> values;
	values["cookie"]  = ConfigValue::FromString("abc=1; def=2");
	values["use_cdn"] = ConfigValue::FromBool(true);
	values["retry"]   = ConfigValue::FromNumber(3);
	CHECK(store.SetConfig("quark", values));

	auto loaded = store.GetConfig("quark");
	CHECK(loaded.size() == 3);
	CHECK(loaded["cookie"].type == ConfigValue::Type::String);
	CHECK(loaded["cookie"].string_value == "abc=1; def=2");
	CHECK(loaded["use_cdn"].type == ConfigValue::Type::Bool);
	CHECK(loaded["use_cdn"].bool_value == true);
	CHECK(loaded["retry"].type == ConfigValue::Type::Number);
	CHECK(loaded["retry"].number_value == 3);

	// 3. 多插件隔离 + 新实例从磁盘读回（持久化）
	std::map<std::string, ConfigValue> other;
	other["token"] = ConfigValue::FromString("xyz");
	CHECK(store.SetConfig("pan123", other));
	gdl::plugin::PluginConfigStore reopened(dir);
	CHECK(reopened.GetConfig("quark").size() == 3);
	auto token = reopened.GetValue("pan123", "token");
	CHECK(token.has_value() && token->string_value == "xyz");

	// 4. RemoveConfig 只删目标插件
	CHECK(store.RemoveConfig("quark"));
	CHECK(store.GetConfig("quark").empty());
	CHECK(store.GetValue("pan123", "token").has_value());

	// 5. 损坏文件按空处理，不崩溃
	{
		std::ofstream f(dir / "plugin_configs.json", std::ios::trunc);
		f << "{ not valid json";
	}
	gdl::plugin::PluginConfigStore corrupted(dir);
	CHECK(corrupted.GetConfig("pan123").empty());
	// 损坏后仍可写入恢复
	CHECK(corrupted.SetConfig("pan123", other));
	CHECK(corrupted.GetValue("pan123", "token").has_value());
}

// 建一个最小插件：parseUrl 返回 gdl.config.get 的值，验证值优先级
static void WriteConfigProbePlugin(const fs::path& plugins_dir) {
	auto dir = plugins_dir / "config-probe";
	fs::create_directories(dir);
	{
		std::ofstream f(dir / "manifest.json", std::ios::trunc);
		f << R"({
			"manifest_version": 1,
			"name": "config-probe",
			"version": "1.0.0",
			"entry": "main.js",
			"type": "netdisk",
			"url_patterns": ["*://example.com*"],
			"permissions": { "http": ["example.com"] },
			"settings": [
				{ "key": "quality", "type": "select", "default": "high",
				  "options": ["high", "low"], "label": "Quality" },
				{ "key": "cookie", "type": "textarea", "role": "token", "label": "Cookie" }
			]
		})";
	}
	{
		std::ofstream f(dir / "main.js", std::ios::trunc);
		f << R"(export default {
			async parseUrl(url, userToken) {
				const quality = gdl.config.get("quality");
				const missing = gdl.config.get("nonexistent");
				return [{
					path: "/probe",
					name: String(quality) + "|" + String(missing) + "|" + String(userToken),
					size: 0, is_dir: false, file_id: "probe"
				}];
			}
		};)";
	}
}

static void TestGdlConfig(const fs::path& dir) {
	auto plugins_dir = dir / "plugins";
	auto data_dir	= dir / "data";
	fs::create_directories(data_dir);
	WriteConfigProbePlugin(plugins_dir);

	auto& manager = gdl::plugin::DownloadPluginManager::Instance();
	CHECK(manager.LoadJsPlugins(plugins_dir.string(), data_dir.string()));
	auto plugin = manager.GetPluginByName("config-probe");
	CHECK(plugin != nullptr);
	if (!plugin) return;

	// 1. 无用户配置：default 生效，未知 key 为 null
	auto files = plugin->ParseUrl("https://example.com", "tok");
	CHECK(files.has_value() && files->size() == 1);
	if (files && !files->empty()) {
		CHECK(files->front().name == "high|null|tok");
	}

	// 2. 用户保存配置后：用户值覆盖 default（gdl.config 每次读透传到 store）
	gdl::plugin::PluginConfigStore store(data_dir);
	std::map<std::string, gdl::plugin::ConfigValue> values;
	values["quality"] = gdl::plugin::ConfigValue::FromString("low");
	CHECK(store.SetConfig("config-probe", values));
	auto files2 = plugin->ParseUrl("https://example.com", "tok");
	CHECK(files2.has_value() && !files2->empty());
	if (files2 && !files2->empty()) {
		CHECK(files2->front().name == "low|null|tok");
	}

	// 3. GetManifestByName：取到完整 manifest（含 settings）
	auto manifest = manager.GetManifestByName("config-probe");
	CHECK(manifest.has_value());
	if (manifest) {
		CHECK(manifest->settings.size() == 2);
		CHECK(manifest->TokenField() != nullptr && manifest->TokenField()->key == "cookie");
	}
	CHECK(!manager.GetManifestByName("no-such-plugin").has_value());
}

int main() {
	auto root = fs::temp_directory_path() / "gdl_plugin_config_test";
	std::error_code ec;
	fs::remove_all(root, ec);

	TestManifestSettings(root / "manifest");
	TestConfigStore(root / "store");
	TestGdlConfig(root / "jsplugin");

	fs::remove_all(root, ec);
	if (g_failures > 0) {
		std::printf("%d check(s) FAILED\n", g_failures);
		return 1;
	}
	std::printf("all checks passed\n");
	return 0;
}
