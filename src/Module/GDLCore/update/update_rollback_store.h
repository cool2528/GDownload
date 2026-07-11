#pragma once

#include <cstdint>
#include <string>

#include "export.h"

namespace gdl::update {
	struct RollbackPersistenceResult { bool ok{false}; std::string error; };
	struct RollbackReadResult { bool ok{false}; std::uint64_t value{0}; std::string error; };
	class GDLCore_API IUpdateRollbackStore {
	   public:
		virtual ~IUpdateRollbackStore() = default;
		virtual RollbackPersistenceResult AcquireInstallationLease() = 0;
		virtual void ReleaseInstallationLease() = 0;
		virtual RollbackReadResult HighestReleaseId() const = 0;
		virtual RollbackPersistenceResult PersistHighestReleaseId(std::uint64_t release_id) = 0;
		virtual RollbackPersistenceResult RestoreHighestReleaseId(std::uint64_t release_id) = 0;
	};
}  // namespace gdl::update
