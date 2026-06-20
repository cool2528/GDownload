#include "baidu_pcs_api.h"
#include <cpr/cpr.h>
#include <algorithm>
#include <boost/url.hpp>
#include <cmath>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <random>
#include <regex>
#include <sstream>
#include <vector>
#include "GDLCore/encoding/encoding.h"

#ifdef _DEBUG
#define IS_DEBUG_MODE true
#else
#define IS_DEBUG_MODE false
#endif

namespace gdl {
    namespace plugin {

        namespace baidu {
            const std::string BAIDU_PCS_URL = "https://pan.baidu.com/api/sharedownload";
            const std::string BAIDU_UA_WEB =
                "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 "
                "Safari/537.36";
            const std::string BAIDU_PAN_HOST			  = "https://pan.baidu.com";
            const std::string UA_KEY					  = "User-Agent";
            const std::string BAIDU_PCS_SHARE_LIST		  = "/share/list";
            const std::string BAIDU_PCS_SHARE_DOWNLOAD	  = "/api/sharedownload";
            const std::string BAIDU_REPORT_USER			  = "/api/report/user";
            const std::string BAIDU_DOWNLOAD_SIGN_REQUEST = "/share/tplconfig";
            const std::string BAIDU_CLIENT_UA =
                "netdisk;P2SP;3.0.0.8;netdisk;11.12.3;ANG-AN00;android-android;10.0;JSbridge4.4.0;jointBridge;1.1.0;";
			const std::string BAIDU_TRANSFER_SHARE_FILE = "/share/transfer";
            const std::string BAIDU_FILE_MANGER			= "/api/filemanager";
			const std::string PCS_BAIDU_COM				= "https://pcs.baidu.com";
			const std::string PCS_APP_ID				= "266719";
			const std::string PAN_APP_ID				= "250528";
        }  // namespace baidu
        namespace detail {

            class DpLogId {
               public:
                explicit DpLogId(const std::string& client = "") : client_(client), count_id_(0) {
                    session_id_ = GetRandomInt(6);
                    user_id_	= "00" + std::to_string(GetRandomInt(8));
                }

                std::string GetDpLogId(const std::string& uk = "") {
                    std::string inner_user_id = user_id_;
                    if (!uk.empty() && ValidateUk(uk)) {
                        inner_user_id = uk;
                    }
                    return client_ + std::to_string(session_id_) + inner_user_id + GetCountId();
                }

                std::string GetDpLogIdByUrl(const std::string& url) {
                    size_t pos = url.find("dp-logid=");
                    if (pos != std::string::npos) {
                        size_t start = pos + 9;	 // "dp-logid=".length()
                        size_t end	 = url.find_first_not_of("0123456789", start);
                        if (end == std::string::npos) {
                            end = url.length();
                        }
                        return url.substr(start, end - start);
                    }
                    return GetDpLogId();
                }

                std::string AddDpLogId(const std::string& url, const std::string& uk = "",
                                       const std::string& dp_log_id = "") {
                    // Check if URL already contains dp-logid
                    if (url.find("dp-logid=") != std::string::npos) {
                        return url;
                    }

                    std::string result = url;
                    std::string log_id = dp_log_id.empty() ? GetDpLogId(uk) : dp_log_id;

                    // Add appropriate connector (? or &)
                    if (url.find('?') != std::string::npos) {
                        result += "&";
                    }
                    else {
                        result += "?";
                    }

                    result += "dp-logid=" + log_id;
                    return result;
                }

               private:
                int GetRandomInt(int num) {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    int min_val = static_cast<int>(std::pow(10, num - 1));
                    int max_val = static_cast<int>(std::pow(10, num)) - 1;
                    std::uniform_int_distribution<int> dist(min_val, max_val);
                    return dist(gen);
                }

                std::string PrefixInteger(int num, int length) {
                    std::string str_num = std::to_string(num);
                    int zeros_needed	= length - static_cast<int>(str_num.length());

                    if (zeros_needed > 0) {
                        return std::string(zeros_needed, '0') + str_num;
                    }
                    return str_num;
                }

                std::string GetCountId() {
                    if (count_id_ < 9999) {
                        count_id_++;
                    }
                    else {
                        count_id_ = 0;
                    }
                    return PrefixInteger(count_id_, 4);
                }

                bool ValidateUk(const std::string& uk) {
                    return uk.length() == 10 && uk.find_first_not_of("0123456789") == std::string::npos;
                }

               private:
                std::string client_;
                int session_id_;
                std::string user_id_;
                int count_id_;
            };

            cpr::Cookies MergeCookies(const cpr::Cookies& cookies1, const cpr::Cookies& cookies2) {
                cpr::Cookies merged_cookies(false);
                std::map<std::string, cpr::Cookie> merged_cookies_map;
                for (const auto& cookie : cookies1) {
                    merged_cookies_map[cookie.GetName()] = cookie;
                }
                for (const auto& cookie : cookies2) {
                    merged_cookies_map[cookie.GetName()] = cookie;
                }
                for (const auto& cookie : merged_cookies_map) {
                    merged_cookies.push_back(cookie.second);
                }
                return merged_cookies;
            }

            bool WriteTextToFile(const std::string& text, const std::string& file_path) {
                std::ofstream file(file_path);
                if (!file.is_open()) {
                    return false;
                }
                file << text;
                file.close();
                return true;
            }

        }  // namespace detail
        BaiduPcsApi::BaiduPcsApi() : cookies_utils_("") {
            InitRequestConfig();
        }

        BaiduPcsApi::~BaiduPcsApi() {}

        void BaiduPcsApi::InitRequestConfig() {
            // 旧实现通过 cpr::priv::set_ssl_option 禁用了证书校验并强制 TLSv1.0，
            // 且 use_debug_settings_ 恒为 false 使该分支永不生效（死代码）。
            // 删除：保留默认的安全 TLS 配置，避免中间人攻击风险与对 cpr 私有 API 的强耦合。
            use_debug_settings_ = false;
        }

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPcsApi::ParseShareUrl(
            const std::string& url, const std::string& user_token) {
			ProcessBaiduCookies(user_token);
            if (!ValidateShareUrl(url)) {
                return std::nullopt;
            }

            boost::url_view share_url(url);
            const auto query_map = share_url.params();
            std::string pwd;
            bool has_password = false;

            auto pwd_it = query_map.find("pwd");
            if (pwd_it != query_map.end()) {
                pwd			 = (*pwd_it).value;
                has_password = true;
            }

            PrepareRequestEnvironment();

            auto initial_response = FetchInitialPage(url);
            if (!initial_response) {
                return std::nullopt;
            }
            std::string new_url = url;
            if (has_password) {
                new_url = GetRedirectUrl(initial_response->header);
                if (new_url.empty()) return std::nullopt;
                auto redirect_response = FetchRedirectPage(new_url);
                if (!redirect_response) {
                    return std::nullopt;
                }
            }

            std::string surl = ExtractSurl(new_url, has_password);
            if (surl.empty()) {
                return std::nullopt;
            }

            bool need_verify = has_password || !pwd.empty();
            if (need_verify && !VerifySharePassword(surl, pwd, new_url)) {
                return std::nullopt;
            }

            auto file_list = FetchShareFileList(surl, new_url);
            if (!file_list.has_value()) {
                return std::nullopt;
            }
            auto file_list_value = file_list.value();
            for (auto& file : file_list_value) {
                file.path = plugin::CookiesUtils::UrlDecode(file.root_path) + "/" + file.name;
            }
            return file_list_value;
        }

        bool BaiduPcsApi::ValidateShareUrl(const std::string& url) const {
            try {
                boost::url_view share_url(url);
                if (share_url.scheme() != "https" || share_url.host() != "pan.baidu.com") {
                    return false;
                }
                return true;
            } catch (const std::exception&) {
                return false;
            }
        }

        void BaiduPcsApi::PrepareRequestEnvironment() {
            ClearCookies();
            log_id_string_.clear();
            rand_sk_string_.clear();
            surl_.clear();
            user_uk_.clear();
            user_share_id_.clear();
            js_token_.clear();
            for (const auto& cookie : cookies_utils_.GetAllCookies()) {
                baidu_cookies_.emplace_back(cpr::Cookie(cookie.first, cookie.second));
            }
        }

        std::optional<cpr::Response> BaiduPcsApi::FetchInitialPage(const std::string& url) {
            cpr::Response reply;

            auto request_params	 = cpr::Url(url);
            auto redirect_params = cpr::Redirect{50, false, false, cpr::PostRedirectFlags::POST_ALL};

            if (use_debug_settings_) {
                reply = cpr::Get(request_params, redirect_params, proxy_settings_, ssl_opts_, baidu_cookies_);
            }
            else {
				reply = cpr::Get(request_params, redirect_params, baidu_cookies_);
            }

            std::string baidu_id;
            baidu_cookies_.encode = false;
            for (const auto& cookie : reply.cookies) {
                if (cookie.GetName() == "BAIDUID") {
                    baidu_id = cookie.GetValue();
                }
                baidu_cookies_.emplace_back(cookie);
            }

            if (baidu_id.empty()) {
                return std::nullopt;
            }

            log_id_string_ = gdl::encoding::StringToBase64(baidu_id);
            return reply;
        }

        std::string BaiduPcsApi::GetRedirectUrl(const cpr::Header& header) const {
            std::string location;
            auto it = header.find("Location");
            if (it != header.end()) {
                location = it->second;
            }
            else {
                return location;
            }
            return baidu::BAIDU_PAN_HOST + location;
        }

        std::optional<cpr::Response> BaiduPcsApi::FetchRedirectPage(const std::string& url) {
            cpr::Response reply;

            auto request_params = cpr::Url(url);
            auto header_params	= cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}};

            if (use_debug_settings_) {
                reply = cpr::Get(request_params, header_params, baidu_cookies_, proxy_settings_, ssl_opts_);
            }
            else {
                reply = cpr::Get(request_params, header_params, baidu_cookies_);
            }

            baidu_cookies_ = detail::MergeCookies(baidu_cookies_, reply.cookies);
            return reply;
        }

        std::string BaiduPcsApi::ExtractSurl(const std::string& url, bool has_password) {
            std::string surl;
            try {
                if (has_password) {
                    auto url_params = boost::url_view(url).params();
                    auto match		= url_params.find("surl");
                    if (match != url_params.end()) {
                        surl  = (*match).value;
                        surl_ = surl;
                        return surl;
                    }
                }

                std::regex pattern1("pan\\.baidu\\.com\\/s\\/1([\\w-]+)");
                std::regex pattern2("pan\\.baidu\\.com\\/share\\/init\\?surl=([\\w-]+)");
                std::regex pattern3("surl=([\\w-]+)");

                std::smatch match;
                if (std::regex_search(url, match, pattern1) && match.size() > 1) {
                    surl = match[1].str();
                }
                else if (std::regex_search(url, match, pattern2) && match.size() > 1) {
                    surl = match[1].str();
                }
                else if (std::regex_search(url, match, pattern3) && match.size() > 1) {
                    surl = match[1].str();
                }
                surl_ = surl;
                return surl;
            } catch (std::exception& e) {
                return "";
            }
            return surl;
        }

        bool BaiduPcsApi::VerifySharePassword(const std::string& surl, const std::string& pwd,
                                              const std::string& referer_url) {
            std::string timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                       std::chrono::system_clock::now().time_since_epoch())
                                                       .count());

            std::string dp_log_id = detail::DpLogId().GetDpLogId();

            cpr::Parameters verify_parameters = {
                {"t", timestamp},	  {"surl", surl},			{"web", "1"},
                {"app_id", "250528"}, {"bdstoken", bds_token_}, {"logid", log_id_string_},
                {"clienttype", "0"},  {"dp-logid", dp_log_id}};

            cpr::Payload verify_payload = {{"pwd", pwd}, {"vcode", ""}, {"vcode_str", ""}};

            std::string verify_url = "https://pan.baidu.com/share/verify";

            cpr::Response reply;
            auto request_params = cpr::Url(verify_url);
            auto header_params	= cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}, {"Referer", referer_url}};

            if (use_debug_settings_) {
                reply = cpr::Post(request_params, header_params, verify_parameters, verify_payload, baidu_cookies_,
                                  proxy_settings_, ssl_opts_);
            }
            else {
                reply = cpr::Post(request_params, header_params, verify_parameters, verify_payload, baidu_cookies_);
            }

            if (reply.status_code != 200) {
                return false;
            }

            try {
                nlohmann::json doc = nlohmann::json::parse(reply.text);
                if (doc.contains("error") && doc["error"] != 0) {
                    return false;
                }
                if (!doc.contains("randsk")) {
                    return false;
                }

                rand_sk_string_ = doc["randsk"].get<std::string>();
            } catch (const std::exception&) {
                return false;
            }

            if (rand_sk_string_.empty()) {
                return false;
            }

            baidu_cookies_ = detail::MergeCookies(baidu_cookies_, reply.cookies);
            return true;
        }

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPcsApi::FetchShareFileList(
            const std::string& surl, const std::string& referer_url) {

            std::string real_url = "https://pan.baidu.com/s/1" + surl;

            cpr::Response reply;
            auto request_params	 = cpr::Url(real_url);
            auto header_params	 = cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}, {"Referer", referer_url}};
            auto request_options = cpr::Redirect{50, false, false, cpr::PostRedirectFlags::POST_ALL};

            if (use_debug_settings_) {
                reply = cpr::Get(request_params, header_params, baidu_cookies_, proxy_settings_, ssl_opts_,
                                 request_options);
            }
            else {
                reply = cpr::Get(request_params, header_params, baidu_cookies_, request_options);
            }

            if (reply.status_code != 200) {
                return std::nullopt;
            }

            if (use_debug_settings_) {
                detail::WriteTextToFile(reply.text, "baidu_share_list.html");
            }
            if (!ExtractShareInfo(reply.text)) {
                return std::nullopt;
            }
            baidu_cookies_ = detail::MergeCookies(baidu_cookies_, reply.cookies);

            if (!ReportUserBehavior(real_url)) {
                return std::nullopt;
            }

            std::string share_list = baidu::BAIDU_PAN_HOST + baidu::BAIDU_PCS_SHARE_LIST;

            cpr::Parameters share_list_parameters = {{"web", "1"},
                                                     {"app_id", "250528"},
                                                     {"desc", "1"},
                                                     {"showempty", "0"},
                                                     {"page", "1"},
                                                     {"num", "100"},
                                                     {"order", "time"},
                                                     {"shorturl", surl},
                                                     {"root", "1"},
                                                     {"view_mode", "1"},
                                                     {"channel", "chunlei"},
                                                     {"bdstoken", bds_token_},
                                                     {"logid", log_id_string_},
                                                     {"clienttype", "0"},
                                                     {"dp-logid", detail::DpLogId().GetDpLogId()}};

            if (use_debug_settings_) {
                reply = cpr::Get(cpr::Url(share_list), share_list_parameters,
                                 cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}, {"Referer", real_url}},
                                 baidu_cookies_, proxy_settings_, ssl_opts_);
            }
            else {
                reply =
                    cpr::Get(cpr::Url(share_list), share_list_parameters,
                             cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}, {"Referer", real_url}}, baidu_cookies_);
            }

            if (reply.status_code != 200) {
                return std::nullopt;
            }

            return ParseFileList(reply.text);
        }

        bool BaiduPcsApi::ReportUserBehavior(const std::string& referer_url) {
            std::string report_user_url = baidu::BAIDU_PAN_HOST + baidu::BAIDU_REPORT_USER;

            cpr::Parameters report_user_parameters = {{"channel", "chunlei"},
                                                      {"web", "1"},
                                                      {"app_id", "250528"},
                                                      {"bdstoken", bds_token_},
                                                      {"logid", gdl::plugin::CookiesUtils::UrlDecode(log_id_string_)},
                                                      {"clienttype", "0"},
                                                      {"dp-logid", detail::DpLogId().GetDpLogId()}};

            cpr::Payload report_user_payload = {{"timestamp", std::to_string(std::time(nullptr))},
                                                {"action", "web_unlogin_share_browse"}};

            cpr::Response reply;
            auto request_params = cpr::Url(report_user_url);
            auto header_params	= cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}, {"Referer", referer_url}};

            if (use_debug_settings_) {
                reply = cpr::Post(request_params, header_params, report_user_parameters, report_user_payload,
                                  baidu_cookies_, proxy_settings_, ssl_opts_);
            }
            else {
                reply = cpr::Post(request_params, header_params, report_user_parameters, report_user_payload,
                                  baidu_cookies_);
            }

            return reply.status_code == 200;
        }

        std::optional<cpr::Response> BaiduPcsApi::FetchDownloadRequestSignature(const std::string& surl,
                                                                                const std::string& referer_url) {
            std::string sig_request_url			   = baidu::BAIDU_PAN_HOST + baidu::BAIDU_DOWNLOAD_SIGN_REQUEST;
            cpr::Parameters sig_request_parameters = {{"surl", surl},
                                                      {"fields", "sign,timestamp"},
                                                      {"view_mode", "1"},
                                                      {"channel", "chunlei"},
                                                      {"web", "1"},
                                                      {"app_id", "250528"},
                                                      {"bdstoken", bds_token_},
                                                      {"logid", log_id_string_},
                                                      {"clienttype", "0"},
                                                      {"dp-logid", detail::DpLogId().GetDpLogId()}};
            auto reply =
                cpr::Get(cpr::Url(sig_request_url), sig_request_parameters,
                         cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}, {"Referer", referer_url}}, baidu_cookies_);
            baidu_cookies_ = detail::MergeCookies(baidu_cookies_, reply.cookies);
            return reply;
        }

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPcsApi::ParseFileList(
            const std::string& json_text) {
            try {
                std::vector<INetDiskDownloadPlugin::FileInfo> result;
                nlohmann::json json = nlohmann::json::parse(json_text);
                if (json.contains("list") && json["list"].is_array()) {
                    for (const auto& item : json["list"]) {
                        INetDiskDownloadPlugin::FileInfo file_info;
						if (item.contains("server_mtime")) {
							if (item["server_mtime"].is_string()) {
								file_info.create_time = std::stoull(item["server_mtime"].get<std::string>());
							}
							else if (item["server_mtime"].is_number()) {
								file_info.create_time = item["server_mtime"].get<std::uint64_t>();
							}
                        }
						if (item.contains("fs_id")) {
							if (item["fs_id"].is_string()) {
								file_info.file_id = item["fs_id"].get<std::string>();
							}
							else if (item["fs_id"].is_number()) {
								file_info.file_id = std::to_string(item["fs_id"].get<std::uint64_t>());
							}
						}
                        if (item.contains("isdir")) {
							if (item["isdir"].is_string()) {
								file_info.is_dir = item["isdir"].get<std::string>() == "1";
                            }
                            else if (item["isdir"].is_number()) {
								file_info.is_dir = item["isdir"].get<std::uint64_t>() == 1;
							}
                        }
                        // 必填字段缺失时跳过该文件，避免单个异常项让整批解析返回 nullopt
                        if (!item.contains("server_filename") || !item["server_filename"].is_string()) {
                            continue;
                        }
                        if (!item.contains("path") || !item["path"].is_string()) {
                            continue;
                        }
                        file_info.name = item["server_filename"].get<std::string>();
                        file_info.path = item["path"].get<std::string>();
						if (item.contains("size")) {
							if (item["size"].is_string()) {
								file_info.size = std::stoull(item["size"].get<std::string>());
                            }
                            else if (item["size"].is_number()) {
								file_info.size = item["size"].get<std::uint64_t>();
							}
						}
                        file_info.type		= file_info.is_dir ? INetDiskDownloadPlugin::FileType::DIR
                                                               : INetDiskDownloadPlugin::FileType::FILE;
						file_info.root_path = root_path_;
                        result.emplace_back(file_info);
                    }
                    if (user_uk_.empty() || user_share_id_.empty()) {
                        if (json.contains("share_id") && json["share_id"].is_number()) {
                            user_share_id_ = std::to_string(json["share_id"].get<std::uint64_t>());
                        }
                        if (json.contains("uk") && json["uk"].is_number()) {
                            user_uk_ = std::to_string(json["uk"].get<std::uint64_t>());
                        }
                    }
                }
                return result;

            } catch (const std::exception&) {
                return std::nullopt;
            }
            return std::nullopt;
        }

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPcsApi::GetShareHomeDirectoryFileList() {
            std::string real_url				  = "https://pan.baidu.com/s/1" + surl_;
            std::string share_list				  = baidu::BAIDU_PAN_HOST + baidu::BAIDU_PCS_SHARE_LIST;
            cpr::Parameters share_list_parameters = {{"web", "1"},
                                                     {"app_id", "250528"},
                                                     {"desc", "1"},
                                                     {"showempty", "0"},
                                                     {"page", "1"},
                                                     {"num", "100"},
                                                     {"order", "time"},
                                                     {"shorturl", surl_},
                                                     {"root", "1"},
                                                     {"view_mode", "1"},
                                                     {"channel", "chunlei"},
                                                     {"bdstoken", bds_token_},
                                                     {"logid", log_id_string_},
                                                     {"clienttype", "0"},
                                                     {"dp-logid", detail::DpLogId().GetDpLogId()}};

            auto reply =
                cpr::Get(cpr::Url(share_list), share_list_parameters,
                         cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}, {"Referer", real_url}}, baidu_cookies_);

            if (reply.status_code != 200) {
                return std::nullopt;
            }

            auto file_list = ParseFileList(reply.text);
            if (!file_list.has_value()) {
                return std::nullopt;
            }
            auto file_list_value = file_list.value();
            for (auto& file : file_list_value) {
                file.path = plugin::CookiesUtils::UrlDecode(file.root_path) + "/" + file.name;
            }
            return file_list_value;
        }

        std::string BaiduPcsApi::ExtractCookies() const {
            plugin::CookiesUtils cookies_utils("");
            for (const auto& cookie : baidu_cookies_) {
                cookies_utils.AppendCookie(cookie.GetName(), cookie.GetValue());
            }
            return cookies_utils.ToString();
        }

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPcsApi::EnterDirectory(
            const INetDiskDownloadPlugin::FileInfo& info) {
            if (info.path == plugin::CookiesUtils::UrlDecode(root_path_)) {
                return GetShareHomeDirectoryFileList();
            }
            else {
                std::string share_list_url			  = baidu::BAIDU_PAN_HOST + baidu::BAIDU_PCS_SHARE_LIST;
                cpr::Parameters share_list_parameters = {
                    {"is_from_web", "true"},
                    {"sekey", plugin::CookiesUtils::UrlDecode(rand_sk_string_)},
                    {"uk", user_uk_},
                    {"shareid", user_share_id_},
                    {"order", "other"},
                    {"desc", "1"},
                    {"showempty", "0"},
                    {"view_mode", "1"},
                    {"web", "1"},
                    {"page", "1"},
                    {"num", "100"},
                    {"dir", info.path},
                    {"t", GenerateRandomFloat()},
                    {"channel", "chunlei"},
                    {"app_id", "250528"},
                    {"bdstoken", bds_token_},
                    {"logid", log_id_string_},
                    {"clienttype", "0"},
                    {"dp-logid", detail::DpLogId().GetDpLogId()},

                };

                std::string real_url = baidu::BAIDU_PAN_HOST + "/s/" + (surl_.starts_with("1") ? surl_ : "1" + surl_);
                cpr::Header header	 = {
                      {baidu::UA_KEY, baidu::BAIDU_UA_WEB},
                      {"Referer", real_url},
                  };
                auto reply = cpr::Get(cpr::Url(share_list_url), share_list_parameters, header, baidu_cookies_);
                if (reply.status_code != 200) {
                    return std::nullopt;
                }
                return ParseFileList(reply.text);
            }
        }

        std::optional<std::vector<INetDiskDownloadPlugin::ParseResult>> BaiduPcsApi::GetDownloadInfo(
            const INetDiskDownloadPlugin::FileInfo& info) {
            if (info.is_dir) {
                auto file_info = EnterDirectory(info);
                if (!file_info.has_value()) {
                    return std::nullopt;
                }
                auto files = file_info.value();
                std::vector<INetDiskDownloadPlugin::ParseResult> results;
                for (const auto& file : files) {
                    auto parse_info_res = GetDownloadInfo(file);
                    if (!parse_info_res.has_value()) {
                        continue;
                    }
                    results.insert(results.end(), parse_info_res.value().begin(), parse_info_res.value().end());
                }
                return results;
            }
            else {
#if 0
                std::string url;
                if (!surl_.starts_with("1")) {
                    url = "1" + surl_;
                }
                std::string referer_url = baidu::BAIDU_PAN_HOST + "/s/" + url;
                auto reply_res			= FetchDownloadRequestSignature(url, referer_url);
                if (!reply_res.has_value()) return std::nullopt;
                auto reply = reply_res.value();
                if (reply.status_code != 200) {
                    return std::nullopt;
                }
                std::string timestamp, sign;
                try {
                    nlohmann::json doc = nlohmann::json::parse(reply.text);
                    if (doc.contains("errno") && doc["errno"].is_number()) {
                        if (doc["errno"].get<int>() != 0) {
                            return std::nullopt;
                        }
                        if (doc.contains("data") && doc["data"].is_object()) {
                            if (doc["data"].contains("sign") && doc["data"]["sign"].is_string()) {
                                sign = doc["data"]["sign"].get<std::string>();
                            }
                            if (doc["data"].contains("timestamp") && doc["data"]["timestamp"].is_number()) {
                                timestamp = std::to_string(doc["data"]["timestamp"].get<std::uint64_t>());
                            }
                        }
                    }
                } catch (std::exception& e) {
                    return std::nullopt;
                }

                cpr::Parameters download_parameters = {{"sign", sign},
                                                       {"timestamp", timestamp},
                                                       {"channel", "chunlei"},
                                                       {"web", "0"},
                                                       {"app_id", "250528"},
                                                       {"bdstoken", bds_token_},
                                                       {"logid", log_id_string_},
                                                       {"clienttype", "1"},
                                                       {"jsToken", js_token_},
                                                       {"dp-logid", detail::DpLogId().GetDpLogId()}};

                std::string extra_str =
                    std::format(R"({{"sekey":"{}"}})", plugin::CookiesUtils::UrlDecode(rand_sk_string_));
                cpr::Payload download_payload	 = {{"encrypt", "0"},
                                                    {"extra", extra_str},
                                                    {"product", "share"},
                                                    {"uk", user_uk_},
                                                    {"primaryid", user_share_id_},
                                                    {"fid_list", "[" + info.file_id + "]"},
                                                    {"path_list", ""},
                                                    {"vip", is_vip_},
													{"bdstoken", bds_token_}};
                std::string download_request_url = baidu::BAIDU_PAN_HOST + baidu::BAIDU_PCS_SHARE_DOWNLOAD;
                if (use_debug_settings_) {
                    reply = cpr::Post(cpr::Url(download_request_url), download_parameters,
                                      cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}, {"Referer", referer_url}},
                                      download_payload, baidu_cookies_, ssl_opts_, proxy_settings_);
                }
                else {
                    reply = cpr::Post(cpr::Url(download_request_url), download_parameters,
                                      cpr::Header{{baidu::UA_KEY, baidu::BAIDU_UA_WEB}, {"Referer", referer_url}},
                                      download_payload, baidu_cookies_);
                }
                if (reply.status_code != 200) return std::nullopt;
                auto cookies_str = ExtractCookies();
                try {
                    nlohmann::json doc = nlohmann::json::parse(reply.text);
                    if (doc.contains("errno") && doc["errno"].is_number()) {
                        if (doc["errno"].get<int>() != 0) {
                            return std::nullopt;
                        }
                        if (doc.contains("list") && doc["list"].is_array()) {
                            std::vector<INetDiskDownloadPlugin::ParseResult> result;
                            for (const auto& item : doc["list"]) {
                                if (!item.is_object()) {
                                    return std::nullopt;
                                }
                                INetDiskDownloadPlugin::ParseResult parse_result;
                                parse_result.file_name = item["server_filename"].get<std::string>();
                                parse_result.file_size = item["size"].get<std::size_t>();
                                auto real_url		   = item["dlink"].get<std::string>();
                                parse_result.real_url  = real_url;
                                parse_result.headers.insert(std::make_pair("Cookie", cookies_str));
                                parse_result.headers.insert(std::make_pair("Referer", referer_url));
                                parse_result.headers.insert(std::make_pair("User-Agent", baidu::BAIDU_UA_WEB));
                                result.emplace_back(parse_result);
                            }
                            return result;
                        }
                    }
                } catch (std::exception& e) {
                    return std::nullopt;
                }
#else
				auto bduss_cookie = cookies_utils_.GetCookie("BDUSS");
				if (bduss_cookie.empty()) {
					bduss_cookie = cookies_utils_.GetCookie("BDUSS_BFESS");
				}
				auto transfer_res = TransferShareFile(info);
				if (!transfer_res.has_value()) return std::nullopt;
                auto parse_result_list = transfer_res.value();
				std::vector<INetDiskDownloadPlugin::ParseResult> result;
                for (const auto& parse_result : parse_result_list) {

					INetDiskDownloadPlugin::ParseResult parse_item;
					parse_item.file_name = std::filesystem::path(parse_result.path).filename().string();
                    auto real_urls		 = GetRealDownloadAddress(parse_result);
					if (real_urls.empty()) continue;
                    parse_item.real_url = real_urls.front();
                    if (parse_item.real_url.empty()) continue;
					parse_item.headers.insert(std::make_pair("header", "Cookie:BDUSS=" + bduss_cookie));
                    if (is_vip_ == "0") {
                        parse_item.headers.insert(std::make_pair("force-http-range", "true"));
                        parse_item.headers.insert(std::make_pair("enable-http-pipelining", "true"));
                        parse_item.headers.insert(std::make_pair("max-connection-per-server", "8"));
                        parse_item.headers.insert(std::make_pair("split", "16"));
                    }
					parse_item.headers.insert(std::make_pair("user-agent", baidu::BAIDU_CLIENT_UA));
					parse_item.headers.insert(std::make_pair("out", parse_item.file_name));
					result.emplace_back(parse_item);
                    DeleteTransferShareFile(parse_result);
				}
                return result;
#endif
            }

            return std::nullopt;
        }

        void BaiduPcsApi::SetVerificationCallback(const INetDiskDownloadPlugin::VerificationCallback& callback) {
            verification_callback_ = callback;
        }

        void BaiduPcsApi::ClearCookies() {
            // cpr::Cookies 没有 clear()，用赋空对象的方式清空，避免逐个 pop_back 的 O(n²) 开销
            baidu_cookies_ = cpr::Cookies{};
        }

        bool BaiduPcsApi::ExtractShareInfo(const std::string& html_content) {
            try {

                std::regex pattern(R"(locals\.mset\((\{.*?\})\);)");
                std::smatch matches;
                if (std::regex_search(html_content, matches, pattern) && matches.size() > 1) {
                    std::string json_str	 = matches[1].str();
                    nlohmann::json json_data = nlohmann::json::parse(json_str);
                    if (json_data.contains("bdstoken") && json_data["bdstoken"].is_string()) {
                        this->bds_token_ = json_data["bdstoken"].get<std::string>();
                    }
                    if (json_data.contains("shareid") && json_data["shareid"].is_number()) {
						user_share_id_ = std::to_string(json_data["shareid"].get<std::uint64_t>());
                    }
                    if (json_data.contains("share_uk") && json_data["share_uk"].is_string()) {
                        user_uk_ = json_data["share_uk"].get<std::string>();
                    }
                    if (json_data.contains("is_vip") && json_data["is_vip"].is_number()) {
                        is_vip_ = std::to_string(json_data["is_vip"].get<std::uint64_t>());
                    }
					if (json_data.contains("file_list") && json_data["file_list"].is_array()) {
						for (const auto& item : json_data["file_list"]) {
							if (item.is_object() && item.contains("parent_path") && item["parent_path"].is_string()) {
								root_path_ = item["parent_path"].get<std::string>();
                                break;
							}
						}
					}
                }
                return ExtractJsToken(html_content) && !bds_token_.empty();
            } catch (const std::exception& e) {
                return false;
            }

            return false;
        }

        bool BaiduPcsApi::ExtractJsToken(const std::string& html_content) {
            try {
                std::regex pattern(R"(fn%28%22(\w+)%22)");
                std::smatch matches;
                if (std::regex_search(html_content, matches, pattern) && matches.size() > 1) {
                    js_token_ = matches[1].str();
                    return true;
                }
            } catch (const std::regex_error& e) {
                return false;
            }

            return false;
        }

        std::string BaiduPcsApi::GenerateRandomFloat() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<double> dist(0.1, 0.9);
            double random_value = dist(gen);
            std::stringstream ss;
            ss << std::fixed << std::setprecision(13) << random_value;
            return ss.str();
        }

        void BaiduPcsApi::ProcessBaiduCookies(const std::string& cookies) {
            cookies_utils_	  = CookiesUtils(cookies);
			auto cookies_list = cookies_utils_.GetAllCookies();
            for (auto it = cookies_list.begin(); it != cookies_list.end();) {
                if (it->first != "BDUSS_BFESS" && it->first != "STOKEN" && it->first != "PANPSC" &&
                    it->first != "BDUSS") {
                    it = cookies_list.erase(it);
                }
                else {
                    ++it;
                }
            }
			cookies_utils_.Clear();
            cookies_utils_	  = CookiesUtils(cookies_list);
            auto bduss_cookie = cookies_utils_.GetCookie("BDUSS");
			if (bduss_cookie.empty()) {
				bduss_cookie = cookies_utils_.GetCookie("BDUSS_BFESS");
            }
			if (!pcs_.IsValidate()) {
				pcs_.InitUserInfo(bduss_cookie);
			}
        }

        std::vector<std::string> BaiduPcsApi::GetRealDownloadAddress(const INetDiskDownloadPlugin::FileInfo& link) {
            auto bduss_cookie = cookies_utils_.GetCookie("BDUSS");
			if (bduss_cookie.empty()) {
				bduss_cookie = cookies_utils_.GetCookie("BDUSS_BFESS");
			}
			if (!pcs_.IsValidate()) {
				pcs_.InitUserInfo(bduss_cookie);
			}
			std::string uid	   = std::to_string(pcs_.GetUserID());
			std::string devuid = calu_md5(bduss_cookie);
			std::transform(devuid.begin(), devuid.end(), devuid.begin(), ::toupper);
			devuid += "|0";
            std::string enc		  = calu_sha1(bduss_cookie);
            std::string timestamp = std::to_string(now_timestamp());
			std::string rand_base = enc + uid + "ebrcUYiuxaZv2XGu7KIYKxUrqfnOfpDF" + timestamp + devuid;
			std::string rand	  = calu_sha1(rand_base);
			std::map<std::string, std::string> params;
			params["apn_id"]	 = "1_0";
			params["app_id"]	 = baidu::PAN_APP_ID;
			params["channel"]	 = "0";
			params["check_blue"] = "1";
			params["clienttype"] = "17";
			params["es"]		 = "1";
			params["esl"]		 = "1";
			params["freeisp"]	 = "0";
			params["method"]	 = "locatedownload";
			params["path"]		 = quote_plus(link.path);
			params["queryfree"]	 = "0";
			params["use"]		 = "0";
			params["ver"]		 = "4.0";
			params["time"]		 = timestamp;
			std::string params_str;
			for (auto it = params.begin(); it != params.end(); ++it) {
				if (it != params.begin()) params_str += "&";
				params_str += it->first + "=" + it->second;
			}

			cpr::Header headers = {{"User-Agent", baidu::BAIDU_CLIENT_UA}, {"Connection", "Keep-Alive"}};
			std::string url		= baidu::PCS_BAIDU_COM + "/rest/2.0/pcs/file?" + params_str;
            url += std::format("&rand={}&devuid={}&cuid={}", rand, devuid, devuid);
			std::vector<std::string> urls;
			auto response = cpr::Get(cpr::Url{url}, headers, cpr::Cookies{{"BDUSS", bduss_cookie}});
			if (response.status_code != 200) return urls;
            try {
				json info = json::parse(response.text);
				if (info.contains("urls") && info["urls"].is_array()) {
                    for (const auto& item : info["urls"]) {
						if (item.contains("url") && item["url"].is_string()) {
							const auto url = item["url"].get<std::string>();
							urls.emplace_back(url);
						}
					}
				}
				else if (info.contains("urls") && info["urls"].is_object()) {
					if (info["urls"].contains("url") && info["urls"]["url"].is_string()) {
						const auto url = info["urls"]["url"].get<std::string>();
						urls.emplace_back(url);
					}
				}
                return urls;

            } catch (std::exception& e) {
				return urls;
            }
			return urls;
        }

        std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> BaiduPcsApi::TransferShareFile(
            const INetDiskDownloadPlugin::FileInfo& info) {
			std::string transfer_url = baidu::BAIDU_PAN_HOST + baidu::BAIDU_TRANSFER_SHARE_FILE;
            cpr::Parameters params	 = {
                  {"shareid", user_share_id_},
				  {"from", user_uk_},
				  {"sekey", plugin::CookiesUtils::UrlDecode(rand_sk_string_)},
                  {"ondup", "newcopy"},
                  {"async", "1"},
                  {"channel", "chunlei"},
                  {"web", "1"},
                  {"app_id", "250528"},
                  {"bdstoken", bds_token_},
				  {"logid", log_id_string_},
                  {"clienttype", "0"},
				  {"dp-logid", detail::DpLogId().GetDpLogId()},
              };
            std::string fs_id_list = "[" + info.file_id + "]";
            cpr::Payload payload   = {{"fsidlist", fs_id_list}, {"path", "/"}};
			cpr::Header headers	   = {
				   {"Connection", "keep-alive"},
				   {"Origin", "https://pan.baidu.com"},
				   {"Referer", "https://pan.baidu.com/disk/main"},
				   {"User-Agent", baidu::BAIDU_UA_WEB},
			   };
			auto response = cpr::Post(cpr::Url(transfer_url), params, payload, headers, baidu_cookies_);
			if (response.status_code != 200) return std::nullopt;
			try {

                nlohmann::json json_data = nlohmann::json::parse(response.text);
                if (json_data.contains("errno") && json_data["errno"].is_number() &&
                    json_data["errno"].get<std::uint64_t>() == 0) {
                    if (json_data.contains("extra") && json_data["extra"].is_object() &&
                        json_data["extra"].contains("list") && json_data["extra"]["list"].is_array()) {
						std::vector<INetDiskDownloadPlugin::FileInfo> file_info_list;
                        for (const auto& item : json_data["extra"]["list"]) {
                            if (item.is_object() && item.contains("to") && item["to"].is_string() &&
                                item.contains("to_fs_id") && item["to_fs_id"].is_number()) {
								INetDiskDownloadPlugin::FileInfo file_info;
								file_info.file_id = std::to_string(item["from_fs_id"].get<std::uint64_t>());
                                file_info.path	  = item["to"].get<std::string>();
								file_info_list.push_back(file_info);
							}
						}
                        return file_info_list;
					}
				}

			} catch (std::exception& e) {
				return std::nullopt;
			}
            return std::nullopt;
        }

        bool BaiduPcsApi::DeleteTransferShareFile(const INetDiskDownloadPlugin::FileInfo& info) {

            std::string file_manger_url = baidu::PCS_BAIDU_COM + "/rest/2.0/pcs/file";
            auto bduss_cookie			= cookies_utils_.GetCookie("BDUSS");
            if (bduss_cookie.empty()) {
                bduss_cookie = cookies_utils_.GetCookie("BDUSS_BFESS");
            }
            if (!pcs_.IsValidate()) {
                pcs_.InitUserInfo(bduss_cookie);
            }
            nlohmann::json data_object;
			nlohmann::json params;
            nlohmann::json list_object = nlohmann::json::array();
			params["path"]				  = info.path;
			list_object.push_back(params);
            data_object["list"]			  = list_object;
			std::string data_str = data_object.dump();
			cpr::Multipart multipart_data = {{"param", cpr::Buffer{data_str.begin(), data_str.end(), {}}}};
            auto response		 = cpr::Post(cpr::Url(file_manger_url),
										cpr::Parameters{{"method", "delete"}, {"app_id", baidu::PCS_APP_ID}},
										cpr::Cookies{{"BDUSS",bduss_cookie},false},
										multipart_data);
			if (response.status_code != 200) return false;
			return true;
        }

    }  // namespace plugin
}  // namespace gdl
