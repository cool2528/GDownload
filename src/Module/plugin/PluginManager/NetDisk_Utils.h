#pragma once

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

namespace gdl {
    namespace plugin {

        class CookiesUtils {
           public:
            explicit CookiesUtils(const std::string& cookies) : cookies_(cookies) { ParseCookies(); }

            explicit CookiesUtils(const std::map<std::string, std::string>& cookies) : cookies_map_(cookies) {
                cookies_ = ToString();
            }

            CookiesUtils(const CookiesUtils& other) : cookies_(other.cookies_), cookies_map_(other.cookies_map_) {}

            CookiesUtils(CookiesUtils&& other) noexcept
                : cookies_(std::move(other.cookies_)), cookies_map_(std::move(other.cookies_map_)) {}

            CookiesUtils& operator=(const CookiesUtils& other) noexcept {
                cookies_	 = other.cookies_;
                cookies_map_ = other.cookies_map_;
                return *this;
            }

            CookiesUtils& operator=(CookiesUtils&& other) noexcept {
                cookies_	 = std::move(other.cookies_);
                cookies_map_ = std::move(other.cookies_map_);
                return *this;
            }

            CookiesUtils& operator+(const CookiesUtils& other) noexcept {
                for (const auto& [key, value] : other.cookies_map_) {
                    cookies_map_[key] = value;
                }
                cookies_ = ToString();
                return *this;
            }

            CookiesUtils& operator-(const CookiesUtils& other) noexcept {
                for (const auto& [key, value] : other.cookies_map_) {
                    cookies_map_.erase(key);
                }
                cookies_ = ToString();
                return *this;
            }

            ~CookiesUtils() {
                cookies_.clear();
                cookies_map_.clear();
            }

            bool HasCookie(const std::string& key) const {
                return cookies_map_.find(TrimString(key)) != cookies_map_.end();
            }

            std::string GetCookie(const std::string& key) const {
                auto it = cookies_map_.find(TrimString(key));
                if (it != cookies_map_.end()) {
                    return it->second;
                }
                return "";
            }

            std::string ToString() const {
                std::stringstream ss;
                bool first = true;
                for (const auto& [key, value] : cookies_map_) {
                    if (!first) {
                        ss << "; ";
                    }
                    ss << key << "=" << value;
                    first = false;
                }
                return ss.str();
            }

            void AppendCookie(const std::string& key, const std::string& value) {
                cookies_map_[TrimString(key)] = TrimString(value);
                cookies_					  = ToString();
            }

            void RemoveCookie(const std::string& key) {
                cookies_map_.erase(key);
                cookies_ = ToString();
            }

            const std::map<std::string, std::string>& GetAllCookies() const { return cookies_map_; }

            void Clear() {
                cookies_map_.clear();
                cookies_.clear();
            }

            size_t Size() const { return cookies_map_.size(); }

            bool IsEmpty() const { return cookies_map_.empty(); }

            operator std::map<std::string, std::string>() const { return cookies_map_; }

            operator std::string() const { return cookies_; }

            static std::string UrlEncode(const std::string& str) {
                std::ostringstream escaped;
                escaped.fill('0');
                escaped << std::hex;

                for (char c : str) {
                    if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
                        escaped << c;
                    }
                    else {
                        escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
                    }
                }

                return escaped.str();
            }

            static std::string UrlDecode(const std::string& str) {
                std::string result;
                result.reserve(str.size());

                for (size_t i = 0; i < str.size(); ++i) {
                    if (str[i] == '%' && i + 2 < str.size()) {
                        int value = 0;
                        std::istringstream is(str.substr(i + 1, 2));
                        if (is >> std::hex >> value) {
                            result += static_cast<char>(value);
                            i += 2;
                        }
                        else {
                            result += '%';
                        }
                    }
                    else if (str[i] == '+') {
                        result += ' ';
                    }
                    else {
                        result += str[i];
                    }
                }

                return result;
            }

           private:
            void ParseCookies() {
                std::stringstream ss(cookies_);
                std::string cookie;
                while (std::getline(ss, cookie, ';')) {
                    std::string::size_type pos = cookie.find('=');
                    if (pos != std::string::npos) {
                        std::string key	  = TrimString(cookie.substr(0, pos));
                        std::string value = TrimString(cookie.substr(pos + 1));
                        if (!key.empty()) {
                            cookies_map_[key] = value;
                        }
                    }
                }
            }

            static std::string TrimString(const std::string& str) {
                auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });

                auto end =
                    std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) { return std::isspace(c); }).base();

                return (start < end) ? std::string(start, end) : std::string();
            }

           private:
            std::string cookies_;
            std::map<std::string, std::string> cookies_map_;
        };


        
    }  // namespace plugin
}  // namespace gdl
