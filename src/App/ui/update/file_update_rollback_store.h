#pragma once
#include <QLockFile>
#include <filesystem>
#include "update/update_rollback_store.h"
namespace gdl::update {
	class FileUpdateRollbackStore final : public IUpdateRollbackStore {
	 public:
		explicit FileUpdateRollbackStore(std::filesystem::path state_path);
		RollbackPersistenceResult AcquireInstallationLease() override;
		void ReleaseInstallationLease() override;
		RollbackReadResult HighestReleaseId() const override;
		RollbackPersistenceResult PersistHighestReleaseId(std::uint64_t release_id) override;
		RollbackPersistenceResult RestoreHighestReleaseId(std::uint64_t release_id) override;
	 private:
		std::filesystem::path state_path_;
		QLockFile lock_;
	};
}
