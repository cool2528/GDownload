#include "installation_gate.h"
#include <algorithm>

#include "platform_package_verifier.h"
#include "update_package_verifier.h"
#include "update_rollback_store.h"

namespace gdl::update {
	InstallationGate::InstallationGate(const IPlatformPackageVerifier& platform_verifier,
		IInstallerLauncher& launcher, IUpdateRollbackStore& rollback_store)
		: platform_verifier_(platform_verifier), launcher_(launcher), rollback_store_(rollback_store) {}

	InstallationResult InstallationGate::Install(const std::filesystem::path& package, std::int64_t expected_size,
		const std::string& expected_sha256, const std::string& expected_signer_pin,
		std::uint64_t release_id, std::stop_token stop_token) {
		const auto lease = rollback_store_.AcquireInstallationLease();
		if (!lease.ok) return {InstallationStatus::kPersistenceFailed, 0, lease.error};
		struct LeaseGuard { IUpdateRollbackStore& store; ~LeaseGuard() { store.ReleaseInstallationLease(); } } guard{rollback_store_};
		const auto initial_state = rollback_store_.HighestReleaseId();
		if (!initial_state.ok)
			return {InstallationStatus::kRollbackStateFailed, 0, initial_state.error};
		if (release_id <= initial_state.value)
			return {InstallationStatus::kRollbackRejected, 0, "update release rollback rejected"};
		const auto package_result = VerifyUpdatePackage(package, expected_size, expected_sha256);
		if (!package_result.ok) return {InstallationStatus::kVerificationFailed, 0, package_result.error};
		const auto platform_result = platform_verifier_.Verify(package, expected_signer_pin);
		if (!platform_result.ok) return {InstallationStatus::kVerificationFailed, 0, platform_result.error};
		auto result = launcher_.LaunchAndWait(package, stop_token);
		if (!result.Succeeded()) return result;
		const auto latest = rollback_store_.HighestReleaseId();
		if (!latest.ok) return {InstallationStatus::kRollbackStateFailed, 0, latest.error};
		const auto persisted = rollback_store_.PersistHighestReleaseId(std::max(latest.value, release_id));
		if (!persisted.ok) return {InstallationStatus::kPersistenceFailed, 0, persisted.error};
		return result;
	}
}  // namespace gdl::update
