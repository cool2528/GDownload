#include "update_manifest.h"

#include "update_url_policy.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>

namespace gdl::update {
	namespace {
		std::optional<std::vector<unsigned char>> DecodeBase64(const std::string& text) {
			if (text.empty() || text.size() % 4 != 0) return std::nullopt;
			std::vector<unsigned char> decoded(text.size() / 4 * 3);
			const int written = EVP_DecodeBlock(decoded.data(),
				reinterpret_cast<const unsigned char*>(text.data()), static_cast<int>(text.size()));
			if (written < 0) return std::nullopt;
			size_t padding = 0;
			if (!text.empty() && text.back() == '=') ++padding;
			if (text.size() > 1 && text[text.size() - 2] == '=') ++padding;
			decoded.resize(static_cast<size_t>(written) - padding);
			return decoded;
		}

		bool IsSha256(const std::string& value) {
			return value.size() == 64 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
				return std::isxdigit(c) != 0;
			});
		}

		bool VerifyEd25519(const std::string& message, const std::string& signature_base64,
			const std::string& public_key_base64) {
			auto signature = DecodeBase64(signature_base64);
			auto public_key = DecodeBase64(public_key_base64);
			if (!signature || !public_key || signature->size() != 64 || public_key->size() != 32) return false;
			using KeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
			using ContextPtr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
			KeyPtr key(EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
				public_key->data(), public_key->size()), EVP_PKEY_free);
			ContextPtr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
			return key && context && EVP_DigestVerifyInit(context.get(), nullptr, nullptr, nullptr, key.get()) == 1 &&
				EVP_DigestVerify(context.get(), signature->data(), signature->size(),
					reinterpret_cast<const unsigned char*>(message.data()), message.size()) == 1;
		}
	}  // namespace

	std::optional<std::string> CanonicalizeManifest(const std::string& json_text) {
		try {
			auto json = nlohmann::json::parse(json_text);
			if (!json.is_object()) return std::nullopt;
			json.erase("signature");
			return json.dump(-1, ' ', false, nlohmann::json::error_handler_t::strict);
		} catch (...) { return std::nullopt; }
	}

	ManifestVerificationResult VerifyUpdateManifest(const std::string& json_text,
		const std::string& public_key_base64, const ManifestPolicy& policy) {
		auto fail = [](std::string error) { return ManifestVerificationResult{false, std::move(error), {}}; };
		if (public_key_base64.empty()) return fail("update public key is not configured");
		try {
			auto json = nlohmann::json::parse(json_text);
			if (!json.is_object() || json.value("schema_version", 0) != 1 || !json.contains("signature") ||
				!json.contains("asset")) return fail("malformed update manifest");
			auto canonical = CanonicalizeManifest(json_text);
			if (!canonical || !VerifyEd25519(*canonical, json.at("signature").get<std::string>(), public_key_base64))
				return fail("invalid update manifest signature");
			UpdateManifest manifest;
			manifest.release_id = json.at("release_id").get<std::uint64_t>();
			manifest.version = json.at("version").get<std::string>();
			manifest.platform = json.at("platform").get<std::string>();
			manifest.published_at = json.at("published_at").get<std::int64_t>();
			manifest.expires_at = json.at("expires_at").get<std::int64_t>();
			manifest.notes = json.value("notes", std::string{});
			const auto& asset = json.at("asset");
			manifest.asset = {asset.at("name").get<std::string>(), asset.at("url").get<std::string>(),
				asset.at("size").get<std::int64_t>(), asset.at("sha256").get<std::string>()};
			if (manifest.release_id <= policy.highest_release_id) return fail("update manifest rollback rejected");
			if (manifest.platform != policy.expected_platform) return fail("wrong update platform");
			if (manifest.expires_at < policy.now || manifest.published_at > policy.now) return fail("expired update manifest");
			if (manifest.asset.size <= 0 || !IsSha256(manifest.asset.sha256)) return fail("invalid update asset digest");
			if (!policy.expected_asset_suffix.empty() && !manifest.asset.name.ends_with(policy.expected_asset_suffix))
				return fail("wrong update asset type");
			if (!ValidateDownloadUrl(manifest.asset.url, policy.allowed_hosts))
				return fail("update asset host is not allowed");
			return {true, {}, std::move(manifest)};
		} catch (...) { return fail("malformed update manifest"); }
	}
}  // namespace gdl::update
