#include "encoding.h"
#include <boost/beast/core/detail/base64.hpp>
#include <boost/locale.hpp>
#if defined(_WIN32)
#include <Windows.h>
#endif
namespace gdl {
	namespace encoding {
		namespace detail{
#if defined(_WIN32)
			std::string GetEncodingFromCodePage(UINT codePage) {

				switch (codePage) {
					case 936:
						return "GBK";      // Simplified Chinese
					case 950:
						return "Big5";     // Traditional Chinese
					case 1252:
						return "Windows-1252";  // Western European Languages
					case 1251:
						return "Windows-1251";  // Russian
					case 932:
						return "Shift_JIS";     // Japanese
					case 949:
						return "EUC-KR";        // Korean
					case 65001:
						return "UTF-8";         // UTF-8
					default:
						return "ISO-8859-1";    // Default fallback to Latin-1
				}

			}
#endif
		};

		std::wstring Utf8ToWString(const std::string& str) {
			return boost::locale::conv::utf_to_utf<wchar_t>(str);
		}

		std::wstring AnsiToWString(const std::string& str) {
#if defined(_WIN32)
			UINT codePage		 = GetACP();
			std::string encoding = detail::GetEncodingFromCodePage(codePage);
			return boost::locale::conv::to_utf<wchar_t>(str, encoding);
#else
			return boost::locale::conv::to_utf<wchar_t>(str, "UTF-8");
#endif

		}

		std::string WStringToUtf8(const std::wstring& wstr) {
			return boost::locale::conv::utf_to_utf<char>(wstr);
		}

		std::string WStringToAnsi(const std::wstring& wstr) {
#if defined(_WIN32)
			UINT codePage		 = GetACP();
			std::string encoding = detail::GetEncodingFromCodePage(codePage);
			return boost::locale::conv::from_utf(wstr, encoding);
#else
			return boost::locale::conv::from_utf(wstr, "UTF-8");
#endif


		}

		std::string AnsiToUtf8(const std::string& str) {
#if defined(_WIN32)
			UINT codePage		 = GetACP();
			std::string encoding = detail::GetEncodingFromCodePage(codePage);
			return boost::locale::conv::to_utf<char>(str, encoding);
#else
			return boost::locale::conv::to_utf<char>(str, "UTF-8");
#endif

		}

		std::string Utf8ToAnsi(const std::string& utf8) {
#if defined(_WIN32)
			UINT codePage		 = GetACP();
			std::string encoding = detail::GetEncodingFromCodePage(codePage);
			return boost::locale::conv::from_utf(utf8, encoding);
#else
			return boost::locale::conv::from_utf(utf8, "UTF-8");
#endif

		}

		std::string StringToBase64(const std::string& str) {
			std::string result;
			result.resize(boost::beast::detail::base64::encoded_size(str.size()));

			size_t encoded_size = boost::beast::detail::base64::encode(result.data(), str.data(), str.size());
			result.resize(encoded_size);
			return result;
		}
		std::string Base64ToString(const std::string& base64) {
			std::string result;
			result.resize(boost::beast::detail::base64::decoded_size(base64.size()));

			size_t decoded_size =
				boost::beast::detail::base64::decode(result.data(), base64.data(), base64.size()).first;
			result.resize(decoded_size);
			return result;
		}
	}  // namespace encoding
}  // namespace gdl
