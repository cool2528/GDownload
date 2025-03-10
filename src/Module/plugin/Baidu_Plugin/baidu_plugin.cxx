#include "baidu_plugin.h"

namespace gdl {
    namespace plugin {

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPlugin::ParseUrl(std::string_view url,
                                                                                           std::string_view password) {
            return std::nullopt;
        }

        BaiduPlugin::BaiduPlugin() {}

        BaiduPlugin::~BaiduPlugin()
        {

        }

        bool BaiduPlugin::CanHandle(const std::string& url) const {
            return false;
        }

        INetDiskDownloadPlugin::PluginMetadata BaiduPlugin::GetPluginMetadata() {
            return PluginMetadata();
        }

        std::optional<INetDiskDownloadPlugin::ParseResult> BaiduPlugin::GetDownloadInfo(const FileInfo& info) {
            return std::nullopt;
        }

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPlugin::EnterDirectory(const FileInfo& info) {
            return std::nullopt;
        }

    }  // namespace plugin
}  // namespace gdl

INetDiskDownloadPlugin* CreatePlugin() {
    return nullptr;
}

void DestroyPlugin(INetDiskDownloadPlugin* plugin) {
    if (plugin) {
        delete plugin;
        plugin = nullptr;
    }
}
