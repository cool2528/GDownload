#pragma once
#include <map>
#include <string>
#include "IDownload_Plugin.h"
#include "NetDisk_Utils.h"
namespace gdl {
    namespace plugin {
        class BaiduPcsApi {
           public:
            explicit BaiduPcsApi(const std::string& cookies);
            ~BaiduPcsApi();
            std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> ParseShareUrl(
                const std::string& url, const std::string& password = "");
            std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> EnterDirectory(
                const INetDiskDownloadPlugin::FileInfo& info);
            std::optional<INetDiskDownloadPlugin::ParseResult> GetDownloadInfo(
                const INetDiskDownloadPlugin::FileInfo& info);
            void SetVerificationCallback(const INetDiskDownloadPlugin::VerificationCallback& callback);

           private:
            INetDiskDownloadPlugin::VerificationCallback verification_callback_{nullptr};
            CookiesUtils cookies_utils_;
        };
    }  // namespace plugin
}  // namespace gdl
