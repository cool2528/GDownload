#include "linux_appimage_gate.h"

#include "update_package_verifier.h"
#include "update_rollback_store.h"
#include "prepared_appimage.h"
#include <algorithm>

namespace gdl::update {
	LinuxAppImageGate::LinuxAppImageGate(const ILinuxAppImageVerifier& verifier,
		ILinuxAppImageLauncher& launcher, IUpdateRollbackStore& rollback_store, bool require_signature)
		: verifier_(verifier), launcher_(launcher), rollback_store_(rollback_store),
		  require_signature_(require_signature) {}

	InstallationResult LinuxAppImageGate::Apply(const std::filesystem::path& package,
		std::int64_t expected_size, const std::string& expected_sha256, std::uint64_t release_id) const {
		const auto lease = rollback_store_.AcquireInstallationLease();
		if (!lease.ok) return {InstallationStatus::kPersistenceFailed, 0, lease.error};
		struct LeaseGuard { IUpdateRollbackStore& store; ~LeaseGuard() { store.ReleaseInstallationLease(); } } guard{rollback_store_};
		const auto initial = rollback_store_.HighestReleaseId();
		if (!initial.ok) return {InstallationStatus::kRollbackStateFailed, 0, initial.error};
		if (release_id <= initial.value)
			return {InstallationStatus::kRollbackRejected, 0, "update release rollback rejected"};
		const auto package_result = VerifyUpdatePackage(package, expected_size, expected_sha256);
		if (!package_result.ok) return {InstallationStatus::kVerificationFailed, 0, package_result.error};
		const auto prepared = CapturePreparedAppImage(package);
		if (!prepared) return {InstallationStatus::kVerificationFailed, 0, "failed to inspect staged AppImage"};
		switch (verifier_.Verify(package)) {
			case AppImageSignatureState::kPassed: break;
			case AppImageSignatureState::kUnsigned:
				if (require_signature_)
					return {InstallationStatus::kVerificationFailed, 0, "unsigned AppImage rejected"};
				break;
			case AppImageSignatureState::kInvalid:
			case AppImageSignatureState::kKeyChanged:
				return {InstallationStatus::kVerificationFailed, 0, "AppImage signature rejected"};
		}
		if (!PreparedAppImageUnchanged(*prepared))
			return {InstallationStatus::kVerificationFailed, 0, "staged AppImage changed after verification"};
		const auto latest = rollback_store_.HighestReleaseId();
		if (!latest.ok) return {InstallationStatus::kRollbackStateFailed, 0, latest.error};
		const auto persisted = rollback_store_.PersistHighestReleaseId(std::max(latest.value, release_id));
		if (!persisted.ok) return {InstallationStatus::kPersistenceFailed, 0, persisted.error};
		auto result = launcher_.Launch(package);
		if (!result.Succeeded()) {
			const auto restored = rollback_store_.RestoreHighestReleaseId(latest.value);
			if (!restored.ok)
				return {InstallationStatus::kPersistenceFailed, result.exit_code,
					"AppImage launch failed and rollback restoration failed: " + restored.error};
			return result;
		}
		return result;
	}
}  // namespace gdl::update
