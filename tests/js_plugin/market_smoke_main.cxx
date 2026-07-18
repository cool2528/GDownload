// 插件市场服务真实冒烟测试
// 用法：market_smoke <registry_url> <plugins_dir> <data_dir> <install_name>
#include <cstdio>
#include <string>

#include "market/plugin_market_service.h"

using namespace gdl::market;

static const char* StateStr(InstallState s) {
	switch (s) {
		case InstallState::Available: return "Available";
		case InstallState::Installed: return "Installed";
		case InstallState::UpdateAvailable: return "UpdateAvailable";
	}
	return "?";
}

static void DumpItems(const PluginMarketService& svc) {
	for (const auto& it : svc.ComputeItems()) {
		std::printf("  - %-16s state=%-15s installed=[%s] latest=%s\n    name=\"%s\" desc=\"%s\"\n",
					it.meta.name.c_str(), StateStr(it.state),
					it.installed_version.empty() ? "-" : it.installed_version.c_str(), it.meta.latest.c_str(),
					it.meta.display_name.c_str(), it.meta.description.c_str());
	}
}

int main(int argc, char** argv) {
	if (argc < 5) {
		std::printf("usage: %s <registry_url> <plugins_dir> <data_dir> <install_name>\n", argv[0]);
		return 2;
	}
	std::string registry_url = argv[1];
	std::string plugins_dir	 = argv[2];
	std::string data_dir	 = argv[3];
	std::string install_name = argv[4];

	PluginMarketService svc(plugins_dir, data_dir);
	// 第 5 个参数可选：locale（如 zh_CN），用于本地化名称/描述
	if (argc >= 6) {
		svc.SetLocale(argv[5]);
		std::printf("locale=%s\n", argv[5]);
	}

	std::printf("== FetchRegistry ==\n");
	std::string err;
	if (!svc.FetchRegistry({registry_url}, err)) {
		std::printf("FAIL: %s\n", err.c_str());
		return 1;
	}
	std::printf("registry plugins=%zu\n", svc.registry().size());

	std::printf("== ComputeItems (before) ==\n");
	DumpItems(svc);

	// 定位要安装的版本
	std::string version;
	for (const auto& p : svc.registry()) {
		if (p.name == install_name) {
			version = p.latest;
		}
	}
	if (version.empty()) {
		std::printf("FAIL: %s not in registry\n", install_name.c_str());
		return 1;
	}

	std::printf("== InstallPlugin %s v%s ==\n", install_name.c_str(), version.c_str());
	if (!svc.InstallPlugin(install_name, version,
						   [](int pct, const std::string& stage) {
							   std::printf("  [%3d%%] %s\n", pct, stage.c_str());
						   },
						   err)) {
		std::printf("FAIL install: %s\n", err.c_str());
		return 1;
	}

	std::printf("== ComputeItems (after install) ==\n");
	DumpItems(svc);

	std::printf("== SetEnabled false ==\n");
	if (!svc.SetEnabled(install_name, false, err)) {
		std::printf("FAIL disable: %s\n", err.c_str());
		return 1;
	}
	DumpItems(svc);

	std::printf("== SetEnabled true ==\n");
	svc.SetEnabled(install_name, true, err);
	DumpItems(svc);

	std::printf("== UninstallPlugin ==\n");
	if (!svc.UninstallPlugin(install_name, err)) {
		std::printf("FAIL uninstall: %s\n", err.c_str());
		return 1;
	}
	DumpItems(svc);

	std::printf("== DONE ==\n");
	return 0;
}
