#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <stop_token>

#include "export.h"

namespace gdl::update {
	enum class InstallationStatus { kSucceeded, kLaunchFailed, kTimedOut, kCancelledOrNonZero, kVerificationFailed,
		kRollbackRejected, kRollbackStateFailed, kPersistenceFailed };
	struct InstallationResult {
		InstallationStatus status{InstallationStatus::kVerificationFailed};
		int exit_code{0};
		std::string detail;
		bool Succeeded() const { return status == InstallationStatus::kSucceeded; }
	};
	class IPlatformPackageVerifier;
	class IUpdateRollbackStore;

	class GDLCore_API IInstallerLauncher {
	   public:
		virtual ~IInstallerLauncher() = default;
		virtual InstallationResult LaunchAndWait(const std::filesystem::path& package, std::stop_token stop_token) = 0;
	};

	class GDLCore_API InstallationGate {
	   public:
		InstallationGate(const IPlatformPackageVerifier& platform_verifier, IInstallerLauncher& launcher,
			IUpdateRollbackStore& rollback_store);

		InstallationResult Install(const std::filesystem::path& package, std::int64_t expected_size,
			const std::string& expected_sha256, const std::string& expected_signer_pin,
			std::uint64_t release_id, std::stop_token stop_token = {});

	   private:
		const IPlatformPackageVerifier& platform_verifier_;
		IInstallerLauncher& launcher_;
		IUpdateRollbackStore& rollback_store_;
	};
}  // namespace gdl::update
