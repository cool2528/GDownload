// JS 插件真实冒烟测试宿主
// 走完整生产路径：DownloadPluginManager::LoadJsPlugins → GetPluginsForUrl → ParseUrl/GetDownloadInfo
// 用法：smoke_main <plugins_dir> <data_dir> <url> [user_token]
#include <cstdio>
#include <string>

#include "IDownload_Plugin.h"
#include "plugin_manager.h"

namespace {
	void PrintFiles(const std::vector<INetDiskDownloadPlugin::FileInfo>& files) {
		std::printf("  files=%zu\n", files.size());
		size_t shown = 0;
		for (const auto& f : files) {
			std::printf("    [%s] name=%s size=%zu id=%s path=%s\n", f.is_dir ? "DIR " : "FILE",
						f.name.c_str(), f.size, f.file_id.c_str(), f.path.c_str());
			if (++shown >= 20) {
				std::printf("    ...\n");
				break;
			}
		}
	}
}  // namespace

int main(int argc, char** argv) {
	if (argc < 4) {
		std::printf("usage: %s <plugins_dir> <data_dir> <url> [user_token]\n", argv[0]);
		return 2;
	}
	std::string plugins_dir = argv[1];
	std::string data_dir	= argv[2];
	std::string url			= argv[3];
	std::string user_token	= argc >= 5 ? argv[4] : "";

	auto& manager = gdl::plugin::DownloadPluginManager::Instance();
	bool loaded	  = manager.LoadJsPlugins(plugins_dir, data_dir);
	std::printf("LoadJsPlugins(%s) -> %s\n", plugins_dir.c_str(), loaded ? "true" : "false");

	auto plugins = manager.GetPluginsForUrl(url);
	std::printf("GetPluginsForUrl(%s) -> %zu plugin(s)\n", url.c_str(), plugins.size());
	if (plugins.empty()) {
		std::printf("FAIL: no plugin matched the url\n");
		return 1;
	}

	auto plugin	  = plugins.front();
	auto metadata = plugin->GetPluginMetadata();
	std::printf("plugin: name=%s version=%s\n", metadata.name.c_str(), metadata.version.c_str());

	// 消息通知回调：打印插件 notify
	plugin->SetMessageNotifyCallback(
		[](std::string_view msg, const INetDiskDownloadPlugin::MsgType& type) {
			std::printf("  [notify:%d] %.*s\n", static_cast<int>(type), static_cast<int>(msg.size()), msg.data());
		});

	std::printf("== ParseUrl ==\n");
	auto parse = plugin->ParseUrl(url, user_token);
	if (!parse) {
		std::printf("FAIL: ParseUrl returned nullopt\n");
		return 1;
	}
	PrintFiles(*parse);
	if (parse->empty()) {
		std::printf("WARN: ParseUrl returned empty list\n");
		return 0;
	}

	// 若首项是目录，先验证 EnterDirectory
	std::vector<INetDiskDownloadPlugin::FileInfo> current = *parse;
	for (int depth = 0; depth < 3; ++depth) {
		const INetDiskDownloadPlugin::FileInfo* dir = nullptr;
		for (const auto& f : current) {
			if (f.is_dir) {
				dir = &f;
				break;
			}
		}
		if (!dir) {
			break;
		}
		std::printf("== EnterDirectory(%s) ==\n", dir->name.c_str());
		auto sub = plugin->EnterDirectory(*dir);
		if (!sub) {
			std::printf("FAIL: EnterDirectory returned nullopt\n");
			return 1;
		}
		PrintFiles(*sub);
		current = *sub;
	}

	// 对第一个文件（非目录优先）尝试 GetDownloadInfo
	const INetDiskDownloadPlugin::FileInfo* target = nullptr;
	for (const auto& f : current) {
		if (!f.is_dir) {
			target = &f;
			break;
		}
	}
	if (!target) {
		target = &current.front();
	}

	std::printf("== GetDownloadInfo(%s) ==\n", target->name.c_str());
	auto dl = plugin->GetDownloadInfo(*target);
	if (!dl) {
		std::printf("NOTE: GetDownloadInfo returned nullopt (expected without login for baidu)\n");
		return 0;
	}
	std::printf("  results=%zu\n", dl->size());
	for (const auto& r : *dl) {
		std::printf("    file=%s size=%zu\n      real_url=%s\n", r.file_name.c_str(), r.file_size,
					r.real_url.c_str());
		for (const auto& h : r.headers) {
			std::printf("      header: %s = %s\n", h.first.c_str(), h.second.c_str());
		}
		for (const auto& m : r.mirrors) {
			std::printf("      mirror: %s\n", m.c_str());
		}
	}
	std::printf("== DONE ==\n");
	return 0;
}
