#include "baidu_plugin.h"
#include "baiduApi/baidu_pcs_api.h"
namespace gdl {
    namespace plugin {

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPlugin::ParseUrl(
            std::string_view url, std::string_view user_token) {
			if (pcs_api_) {
                return pcs_api_->ParseShareUrl(url.data(), user_token.data());
			}
            return std::nullopt;
        }

        BaiduPlugin::BaiduPlugin() {
            pcs_api_ = std::make_unique<BaiduPcsApi>();
        }

        BaiduPlugin::~BaiduPlugin() {}

        bool BaiduPlugin::CanHandle(const std::string& url) const {
			return url.find("pan.baidu.com") != std::string::npos;
        }

        INetDiskDownloadPlugin::PluginMetadata BaiduPlugin::GetPluginMetadata() {
			PluginMetadata metadata;
            metadata.author		 = "GDL";
			metadata.description = "BaiduNetDisk";
            metadata.name		 = "BaiduNetDisk";
            metadata.version	 = "1.0.0";
			return metadata;
        }

        std::optional<std::vector<INetDiskDownloadPlugin::ParseResult>> BaiduPlugin::GetDownloadInfo(
			const FileInfo& info) {
			if (pcs_api_) {
				return pcs_api_->GetDownloadInfo(info);
			}
            return std::nullopt;
        }

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPlugin::EnterDirectory(const FileInfo& info) {
			if (pcs_api_) {
				return pcs_api_->EnterDirectory(info);
			}
            return std::nullopt;
        }

    }  // namespace plugin
}  // namespace gdl

INetDiskDownloadPlugin* CreatePlugin() {
    return new gdl::plugin::BaiduPlugin();
}

void DestroyPlugin(INetDiskDownloadPlugin* plugin) {
    if (plugin) {
        delete plugin;
        plugin = nullptr;
    }
}
