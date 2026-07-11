#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "export.h"

namespace gdl::update {
	struct UpdateAssetManifest {
		std::string name;
		std::string url;
		std::int64_t size{0};
		std::string sha256;
	};

	struct UpdateManifest {
		std::uint64_t release_id{0};
		std::string version;
		std::string platform;
		std::int64_t published_at{0};
		std::int64_t expires_at{0};
		std::string notes;
		UpdateAssetManifest asset;
	};

	struct ManifestPolicy {
		std::string expected_platform;
		std::string expected_asset_suffix;
		std::vector<std::string> allowed_hosts;
		std::uint64_t highest_release_id{0};
		std::int64_t now{0};
	};

	struct ManifestVerificationResult {
		bool ok{false};
		std::string error;
		UpdateManifest manifest;
	};

	GDLCore_API std::optional<std::string> CanonicalizeManifest(const std::string& json_text);
	GDLCore_API ManifestVerificationResult VerifyUpdateManifest(const std::string& json_text,
		const std::string& public_key_base64, const ManifestPolicy& policy);
}  // namespace gdl::update
