#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "export.h"

namespace gdl::update {
	enum class RedirectDecision { kFollow, kReject };

	GDLCore_API bool ValidateDownloadUrl(std::string_view url,
		const std::vector<std::string>& allowed_hosts);
	GDLCore_API RedirectDecision DecideDownloadRedirect(std::string_view redirect_url,
		const std::vector<std::string>& allowed_hosts);
}  // namespace gdl::update
