#pragma once

#include <algorithm>
#include <cwctype>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace gdl {
	namespace engine {

		inline std::string WindowsChromeUserAgent() {
			return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
				   "Chrome/120.0.0.0 Safari/537.36";
		}

		inline std::string WindowsEdgeUserAgent() {
			return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
				   "Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0";
		}

		inline std::string WindowsFirefoxUserAgent() {
			return "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:121.0) Gecko/20100101 Firefox/121.0";
		}

		inline std::string MacSafariUserAgent() {
			return "Mozilla/5.0 (Macintosh; Intel Mac OS X 14_1) AppleWebKit/605.1.15 (KHTML, like Gecko) "
				   "Version/17.0 Safari/605.1.15";
		}

		inline std::string LinuxChromeUserAgent() {
			return "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
				   "Chrome/120.0.0.0 Safari/537.36";
		}

#if defined(_WIN32)
		inline std::wstring ReadWindowsDefaultBrowserProgId() {
			wchar_t value[512] = {};
			DWORD value_size = sizeof(value);
			const auto status = RegGetValueW(
				HKEY_CURRENT_USER,
				L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice",
				L"ProgId", RRF_RT_REG_SZ, nullptr, value, &value_size);
			if (status != ERROR_SUCCESS) {
				return {};
			}
			return value;
		}

		inline bool ContainsCaseInsensitive(std::wstring text, std::wstring needle) {
			const auto to_lower = [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); };
			std::transform(text.begin(), text.end(), text.begin(), to_lower);
			std::transform(needle.begin(), needle.end(), needle.begin(), to_lower);
			return text.find(needle) != std::wstring::npos;
		}
#endif

		inline std::string DefaultBrowserUserAgentString() {
#if defined(_WIN32)
			const std::wstring prog_id = ReadWindowsDefaultBrowserProgId();
			if (ContainsCaseInsensitive(prog_id, L"firefox")) {
				return WindowsFirefoxUserAgent();
			}
			if (ContainsCaseInsensitive(prog_id, L"chrome")) {
				return WindowsChromeUserAgent();
			}
			return WindowsEdgeUserAgent();
#elif defined(__APPLE__)
			return MacSafariUserAgent();
#else
			return LinuxChromeUserAgent();
#endif
		}

	}  // namespace engine
}  // namespace gdl
