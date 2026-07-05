#include "encoding.h"
#include <openssl/evp.h>
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
			// OpenSSL EVP 实现，避免依赖 boost::beast::detail 非公共 API（T6）
			if (str.empty()) return {};
			const int in_len = static_cast<int>(str.size());
			std::string result;
			result.resize(static_cast<size_t>(4 * ((in_len + 2) / 3)));
			const int out_len = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(result.data()),
				                                    reinterpret_cast<const unsigned char*>(str.data()), in_len);
			result.resize(out_len < 0 ? 0 : static_cast<size_t>(out_len));
			return result;
		}
		std::string Base64ToString(const std::string& base64) {
			if (base64.empty()) return {};
			const int in_len = static_cast<int>(base64.size());
			std::string result;
			result.resize(static_cast<size_t>(3 * (in_len / 4)));
			int out_len = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(result.data()),
				                              reinterpret_cast<const unsigned char*>(base64.data()), in_len);
			if (out_len < 0) return {};
			// EVP_DecodeBlock 不剥离 '=' 填充产生的尾部零字节，按填充数修正（T6）
			if (base64.back() == '=') --out_len;
			if (base64.size() >= 2 && base64[base64.size() - 2] == '=') --out_len;
			result.resize(out_len < 0 ? 0 : static_cast<size_t>(out_len));
			return result;
		}
	}  // namespace encoding
}  // namespace gdl
