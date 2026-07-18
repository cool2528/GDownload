// 插件配置系统单元测试：manifest settings schema / PluginConfigStore / gdl.config / GetManifestByName
// 无网络依赖；临时目录建在系统 temp 下，进程退出前清理
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "JsPluginRuntime/plugin_manifest.h"

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
}

int main() {
	auto root = fs::temp_directory_path() / "gdl_plugin_config_test";
	std::error_code ec;
	fs::remove_all(root, ec);

	TestManifestSettings(root / "manifest");

	fs::remove_all(root, ec);
	if (g_failures > 0) {
		std::printf("%d check(s) FAILED\n", g_failures);
		return 1;
	}
	std::printf("all checks passed\n");
	return 0;
}
