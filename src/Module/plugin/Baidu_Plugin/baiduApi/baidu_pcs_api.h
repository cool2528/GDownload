#pragma once
#include <cpr/cpr.h>
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
            std::optional<std::vector<INetDiskDownloadPlugin::ParseResult>> GetDownloadInfo(
                const INetDiskDownloadPlugin::FileInfo& info);
            void SetVerificationCallback(const INetDiskDownloadPlugin::VerificationCallback& callback);

           private:
            void InitRequestConfig();

            bool ValidateShareUrl(const std::string& url) const;

            void PrepareRequestEnvironment();

            std::optional<cpr::Response> FetchInitialPage(const std::string& url);

            std::string GetRedirectUrl(const cpr::Header& header) const;

            std::optional<cpr::Response> FetchRedirectPage(const std::string& url);

            std::string ExtractSurl(const std::string& url, bool has_password = true);

            bool VerifySharePassword(const std::string& surl, const std::string& pwd, const std::string& referer_url);

            std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> FetchShareFileList(
                const std::string& surl, const std::string& referer_url);

            bool ReportUserBehavior(const std::string& referer_url);
            std::optional<cpr::Response> FetchDownloadRequestSignature(const std::string& surl,
                                                                       const std::string& referer_url);

            std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> ParseFileList(const std::string& json_text);

            std::string ExtractCookies() const;

            void ClearCookies();
            bool ExtractShareInfo(const std::string& html_content);
            bool ExtractJsToken(const std::string& html_content);
            static std::string GenerateRandomFloat();

           private:
            INetDiskDownloadPlugin::VerificationCallback verification_callback_{nullptr};
            CookiesUtils cookies_utils_;
            std::string log_id_string_;
            cpr::Cookies baidu_cookies_;
            std::string rand_sk_string_;

            cpr::SslOptions ssl_opts_;
            cpr::Proxies proxy_settings_;
            bool use_debug_settings_ = false;
            std::string surl_;
            std::string user_uk_;
            std::string user_share_id_;
            std::string js_token_;
            std::string bds_token_;
            std::string is_vip_;
        };
    }  // namespace plugin
}  // namespace gdl
