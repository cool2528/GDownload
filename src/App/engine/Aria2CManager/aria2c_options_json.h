#pragma once
#include <rapidjson/document.h>

#include <string>
#include <unordered_map>

#include "Engine_export.h"

namespace gdl {
	namespace engine {

		// 将 options multimap 序列化为 aria2 JSON-RPC 的 options 对象成员。
		// aria2 的 JSON 解析对重复键只保留一条,因此同名多值键(如 header 的
		// Cookie/Referer 多条请求头)必须聚合为字符串数组;单值键保持字符串。
		Engine_API void AppendAria2OptionsJson(const std::unordered_multimap<std::string, std::string>& options,
											   rapidjson::Value& options_value,
											   rapidjson::Document::AllocatorType& allocator);

	}  // namespace engine
}  // namespace gdl
