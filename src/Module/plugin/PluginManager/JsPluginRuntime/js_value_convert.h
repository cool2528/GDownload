#pragma once
#include <quickjs.h>

#include <optional>
#include <vector>

#include "../IDownload_Plugin.h"

namespace gdl {
	namespace plugin {
		namespace js {

			// FileInfo → JS 对象（传给插件的 enterDirectory/getDownloadInfo 参数）
			JSValue FileInfoToJs(JSContext* ctx, const INetDiskDownloadPlugin::FileInfo& info);

			// JS 对象 → FileInfo；字段缺失取默认值
			std::optional<INetDiskDownloadPlugin::FileInfo> JsToFileInfo(JSContext* ctx, JSValueConst value);

			// JS 对象 → ParseResult；real_url 缺失视为无效
			std::optional<INetDiskDownloadPlugin::ParseResult> JsToParseResult(JSContext* ctx, JSValueConst value);

			// JS 数组 → FileInfo 列表；非法元素跳过并记日志
			std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> JsToFileInfoVector(JSContext* ctx,
																							JSValueConst value);

			// JS 数组 → ParseResult 列表
			std::optional<std::vector<INetDiskDownloadPlugin::ParseResult>> JsToParseResultVector(JSContext* ctx,
																								  JSValueConst value);

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl
