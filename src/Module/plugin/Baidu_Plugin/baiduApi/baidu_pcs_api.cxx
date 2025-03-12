#include "baidu_pcs_api.h"
#include <cpr/cpr.h>
#include <boost/url.hpp>
namespace gdl {
    namespace plugin {

        BaiduPcsApi::BaiduPcsApi(const std::string& cookies) : cookies_utils_(cookies) {}

        BaiduPcsApi::~BaiduPcsApi() {}

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPcsApi::ParseShareUrl(
            const std::string& url, const std::string& password) {

            boost::url_view share_url(url);
            if (share_url.scheme() != "https" || share_url.host() != "pan.baidu.com") {
                return std::nullopt;
            }
            const auto query = share_url.query();
            if (query.empty()) {
                return std::nullopt;
            }
            const auto query_map = share_url.params();
            std::string pwd		 = password;
            bool has_password	 = false;
            auto pwd_it			 = query_map.find("pwd");
            if (pwd_it != query_map.end()) {
                pwd			 = (*pwd_it).value;
                has_password = true;
            }
            cpr::Cookies cookies;
            for (const auto& cookie : cookies_utils_.GetAllCookies()) {
                cookies.emplace_back(cpr::Cookie(cookie.first, cookie.second));
            }
            auto reply = cpr::Get(cpr::Url(url), cpr::Redirect{0, false, false, cpr::PostRedirectFlags::POST_ALL},
                                  cpr::Cookies(cookies));

            return std::nullopt;
        }

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPcsApi::EnterDirectory(
            const INetDiskDownloadPlugin::FileInfo& info) {
            return std::nullopt;
        }

        std::optional<INetDiskDownloadPlugin::ParseResult> BaiduPcsApi::GetDownloadInfo(
            const INetDiskDownloadPlugin::FileInfo& info) {
            return std::nullopt;
        }

        void BaiduPcsApi::SetVerificationCallback(const INetDiskDownloadPlugin::VerificationCallback& callback) {}

    }  // namespace plugin
}  // namespace gdl
