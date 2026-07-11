#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "export.h"
#include "installation_gate.h"

namespace gdl::update {
	enum class AppImageSignatureState { kPassed, kUnsigned, kInvalid, kKeyChanged };

	class GDLCore_API ILinuxAppImageVerifier {
	   public:
		virtual ~ILinuxAppImageVerifier() = default;
		virtual AppImageSignatureState Verify(const std::filesystem::path& package) const = 0;
	};

	class GDLCore_API ILinuxAppImageLauncher {
	   public:
		virtual ~ILinuxAppImageLauncher() = default;
		virtual InstallationResult Launch(const std::filesystem::path& package) = 0;
	};
	class IUpdateRollbackStore;

	class GDLCore_API LinuxAppImageGate {
	   public:
		LinuxAppImageGate(const ILinuxAppImageVerifier& verifier, ILinuxAppImageLauncher& launcher,
			IUpdateRollbackStore& rollback_store, bool require_signature);
		InstallationResult Apply(const std::filesystem::path& package, std::int64_t expected_size,
			const std::string& expected_sha256, std::uint64_t release_id) const;

	   private:
		const ILinuxAppImageVerifier& verifier_;
		ILinuxAppImageLauncher& launcher_;
		IUpdateRollbackStore& rollback_store_;
		bool require_signature_{false};
	};
}  // namespace gdl::update
