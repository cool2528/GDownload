#pragma once
#include "Baidu_Plugin_export.h"
#include "PluginManager/IDownload_Plugin.h"
namespace gdl {
    namespace plugin {
		class BaiduPcsApi;
        class BaiduPlugin : public INetDiskDownloadPlugin {
           public:
            explicit BaiduPlugin();
            ~BaiduPlugin() override;
            std::optional<std::vector<FileInfo>> ParseUrl(std::string_view url,
                                                          std::string_view password = "") override;

            std::optional<std::vector<FileInfo>> EnterDirectory(const FileInfo& info) override;

            std::optional<ParseResult> GetDownloadInfo(const FileInfo& info) override;

            PluginMetadata GetPluginMetadata() override;

            bool CanHandle(const std::string& url) const override;
        private:
			std::unique_ptr<BaiduPcsApi> pcs_api_ {nullptr};
        };
    }  // namespace plugin
}  // namespace gdl

#ifdef __cplusplus
extern "C" {
#endif
Baidu_Plugin_API INetDiskDownloadPlugin* CreatePlugin();

Baidu_Plugin_API void DestroyPlugin(INetDiskDownloadPlugin* plugin);
#ifdef __cplusplus
}
#endif
