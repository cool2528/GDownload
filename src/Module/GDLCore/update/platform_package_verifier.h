#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "export.h"
#include "update_package_verifier.h"

namespace gdl::update {
	class GDLCore_API IPlatformPackageVerifier {
	   public:
		virtual ~IPlatformPackageVerifier() = default;
		virtual PackageVerificationResult Verify(const std::filesystem::path& package,
			const std::string& expected_signer_pin) const = 0;
	};

	GDLCore_API bool SignerPinMatches(const std::string& expected_pin, const std::string& actual_spki_sha256);
	GDLCore_API std::unique_ptr<IPlatformPackageVerifier> CreatePlatformPackageVerifier();
}  // namespace gdl::update
