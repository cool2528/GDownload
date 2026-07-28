#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "export.h"

namespace gdl::update {
	enum class RedirectDecision { kFollow, kReject };

	// GitHub 加速镜像（ghproxy 风格：整条原始 URL 拼接在前缀之后）
	inline constexpr std::string_view kGithubMirrorPrefix = "https://gh-proxy.com/";
	inline constexpr std::string_view kGithubMirrorHost	  = "gh-proxy.com";

	GDLCore_API bool ValidateDownloadUrl(std::string_view url,
		const std::vector<std::string>& allowed_hosts);
	GDLCore_API RedirectDecision DecideDownloadRedirect(std::string_view redirect_url,
		const std::vector<std::string>& allowed_hosts);

	// 更新包下载允许的 host 白名单：覆盖 GitHub 释放资产新旧域
	// (objects / release-assets.githubusercontent.com)与官网域；
	// 开启加速时额外放行镜像域。入口校验、重定向链、下载完成终验必须用同一份。
	GDLCore_API std::vector<std::string> BuildAllowedDownloadHosts(bool use_mirror);

	// 根据加速开关生成实际下载 URL：开启且为 GitHub 资源(严格按 host 判定)时
	// 加镜像前缀；已带前缀或非 GitHub 资源原样返回。
	GDLCore_API std::string ResolveDownloadUrl(const std::string& original_url, bool use_mirror);
}  // namespace gdl::update
