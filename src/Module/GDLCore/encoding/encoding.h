#pragma once
#include <string>
#include "export.h"
namespace gdl {
	namespace encoding {
		/**
         * @brief string to wstring
         **/
		GDLCore_API std::wstring Utf8ToWString(const std::string& str);
		GDLCore_API std::wstring AnsiToWString(const std::string& str);
		/**
         * @brief wstring to string
         **/
		GDLCore_API std::string WStringToUtf8(const std::wstring& wstr);
		GDLCore_API std::string WStringToAnsi(const std::wstring& wstr);
		/**
         * @brief string to utf8
         **/
		GDLCore_API std::string AnsiToUtf8(const std::string& str);
		/**
         * @brief utf8 to string
         **/
		GDLCore_API std::string Utf8ToAnsi(const std::string& utf8);

		/**
         * @brief string to base64
         **/
		GDLCore_API std::string StringToBase64(const std::string& str);
		/**
         * @brief base64 to string
         **/
		GDLCore_API std::string Base64ToString(const std::string& base64);
	}  // namespace encoding
}  // namespace gdl