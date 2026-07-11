#include "file_update_rollback_store.h"
#include <charconv>
#include <fstream>
#include "filesystem/atomic_file_replace.h"
namespace gdl::update {
	FileUpdateRollbackStore::FileUpdateRollbackStore(std::filesystem::path state_path)
		: state_path_(std::move(state_path)), lock_(QString::fromStdString((state_path_.parent_path() / "update-install.lock").string())) {
		lock_.setStaleLockTime(60 * 60 * 1000);
	}
	RollbackPersistenceResult FileUpdateRollbackStore::AcquireInstallationLease() {
		std::error_code ec;
		std::filesystem::create_directories(state_path_.parent_path(), ec);
		if (ec) return {false, "failed to create update state directory"};
		return lock_.tryLock(0) ? RollbackPersistenceResult{true, {}} : RollbackPersistenceResult{false, "another update installation is active"};
	}
	void FileUpdateRollbackStore::ReleaseInstallationLease() { lock_.unlock(); }
	RollbackReadResult FileUpdateRollbackStore::HighestReleaseId() const {
		std::error_code ec;
		const bool exists = std::filesystem::exists(state_path_, ec);
		if (ec) return {false, 0, "failed to inspect update rollback state"};
		if (!exists) return {true, 0, {}};
		std::ifstream input(state_path_, std::ios::binary);
		if (!input) return {false, 0, "failed to open update rollback state"};
		std::string text((std::istreambuf_iterator<char>(input)), {});
		if (!text.empty() && text.back() == '\n') text.pop_back();
		std::uint64_t value = 0;
		const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
		if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
			return {false, 0, "corrupt update rollback state"};
		return {true, value, {}};
	}
	RollbackPersistenceResult FileUpdateRollbackStore::PersistHighestReleaseId(std::uint64_t release_id) {
		const auto current = HighestReleaseId();
		if (!current.ok) return {false, current.error};
		const auto value = std::max(current.value, release_id);
		const auto result = gdl::filesystem::AtomicFileReplace(state_path_, [value](std::ostream& output) {
			output << value << '\n';
			return output ? gdl::Result<void>(gdl::Error(0, {})) :
				gdl::Result<void>(gdl::MakeFail(1, "failed to write rollback state"));
		});
		return result.HasError() ? RollbackPersistenceResult{false, result.GetError().what()} : RollbackPersistenceResult{true, {}};
	}
}
