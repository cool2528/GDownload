#include "sqlite_connection.h"

#include <sqlite3.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "logger.h"

namespace gdl::cache {
	namespace {
		thread_local std::vector<const SqliteConnection*> active_connections;

		std::filesystem::path Utf8ToFilesystemPath(const std::string& path) {
#ifdef _WIN32
			if (path.empty()) return {};
			const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(),
				static_cast<int>(path.size()), nullptr, 0);
			if (size <= 0) return std::filesystem::path(path);
			std::wstring wide(static_cast<std::size_t>(size), L'\0');
			MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(),
				static_cast<int>(path.size()), wide.data(), size);
			return std::filesystem::path(wide);
#else
			return std::filesystem::u8path(path);
#endif
		}

		bool ExecuteSql(sqlite3* db, const char* sql) {
			return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
		}

		enum class WalResult { kEnabled, kUnavailable, kError };

		WalResult EnableWal(sqlite3* db) {
			sqlite3_stmt* statement = nullptr;
			if (sqlite3_prepare_v2(db, "PRAGMA journal_mode=WAL", -1, &statement, nullptr) != SQLITE_OK) {
				return WalResult::kError;
			}
			const int step_code = sqlite3_step(statement);
			bool enabled = false;
			if (step_code == SQLITE_ROW) {
				const auto* mode = sqlite3_column_text(statement, 0);
				enabled = mode && sqlite3_stricmp(reinterpret_cast<const char*>(mode), "wal") == 0;
			}
			sqlite3_finalize(statement);
			if (step_code != SQLITE_ROW) return WalResult::kError;
			return enabled ? WalResult::kEnabled : WalResult::kUnavailable;
		}

	}  // namespace

	SqliteConnection::~SqliteConnection() {
		const auto result = Close();
		if (result.HasError()) {
			LOG_WARN("Failed to close sqlite connection during destruction: {}",
				result.GetError().Describe());
		}
	}

	CacheResult<void> SqliteConnection::Open(const std::string& path, SqliteOpenOptions options) {
		ThrowIfReentrant();
		std::lock_guard lock(mutex_);
		if (db_) {
			auto closed = CloseUnlocked();
			if (closed.HasError()) return closed;
		}

		std::error_code directory_error;
		const std::filesystem::path database_path = Utf8ToFilesystemPath(path);
		const std::filesystem::path parent = database_path.parent_path();
		if (!parent.empty()) {
			std::filesystem::create_directories(parent, directory_error);
			if (directory_error) {
				return CacheResult<void>::Failure({.operation = CacheOperation::kCreateDirectory,
					.primary_code = directory_error.value(),
					.extended_code = directory_error.value(),
					.path = path,
					.context = directory_error.message()});
			}
		}

		sqlite3* candidate = nullptr;
		const int open_code = sqlite3_open_v2(path.c_str(), &candidate,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
		if (open_code != SQLITE_OK) {
			const CacheError error = MakeError(CacheOperation::kOpen, path, "sqlite3_open_v2", candidate);
			if (candidate) sqlite3_close_v2(candidate);
			return CacheResult<void>::Failure(error);
		}

		auto fail_configuration = [&](const std::string& context) {
			const CacheError error = MakeError(CacheOperation::kConfigure, path, context, candidate);
			sqlite3_close_v2(candidate);
			return CacheResult<void>::Failure(error);
		};

		if (sqlite3_extended_result_codes(candidate, 1) != SQLITE_OK) {
			return fail_configuration("enable extended result codes");
		}
		if (sqlite3_busy_timeout(candidate, options.busy_timeout_ms) != SQLITE_OK) {
			return fail_configuration("configure busy timeout");
		}
		if (options.request_wal) {
			const WalResult wal_result = EnableWal(candidate);
			if (wal_result == WalResult::kEnabled) {
				if (!ExecuteSql(candidate, "PRAGMA synchronous=NORMAL")) {
					return fail_configuration("configure synchronous=NORMAL");
				}
			} else if (wal_result == WalResult::kUnavailable) {
				LOG_WARN("SQLite WAL is unavailable for {}, using rollback journal", path);
			} else {
				return fail_configuration("configure journal_mode=WAL");
			}
		}

		db_ = candidate;
		path_ = path;
		return CacheResult<void>::Success();
	}

	CacheResult<void> SqliteConnection::Close() {
		ThrowIfReentrant();
		std::lock_guard lock(mutex_);
		return CloseUnlocked();
	}

	CacheResult<void> SqliteConnection::CloseUnlocked() {
		if (!db_) {
			path_.clear();
			return CacheResult<void>::Success();
		}
		const int close_code = sqlite3_close_v2(db_);
		if (close_code != SQLITE_OK) {
			return CacheResult<void>::Failure(MakeError(CacheOperation::kClose, path_,
				"sqlite3_close_v2", db_));
		}
		db_ = nullptr;
		path_.clear();
		return CacheResult<void>::Success();
	}

	bool SqliteConnection::IsOpen() const {
		ThrowIfReentrant();
		std::lock_guard lock(mutex_);
		return db_ != nullptr;
	}

	std::string SqliteConnection::Path() const {
		ThrowIfReentrant();
		std::lock_guard lock(mutex_);
		return path_;
	}

	void SqliteConnection::ThrowIfReentrant() const {
		if (std::find(active_connections.begin(), active_connections.end(), this) !=
			active_connections.end()) {
			throw std::logic_error("SqliteConnection cannot be re-entered from its Use callback");
		}
	}

	void SqliteConnection::EnterUse() const { active_connections.push_back(this); }

	void SqliteConnection::LeaveUse() const { active_connections.pop_back(); }

	CacheError SqliteConnection::MakeClosedError(CacheOperation operation, std::string context) const {
		return {.operation = operation,
			.primary_code = SQLITE_MISUSE,
			.extended_code = SQLITE_MISUSE,
			.path = path_,
			.context = std::move(context)};
	}

	CacheError SqliteConnection::MakeError(CacheOperation operation, const std::string& path,
		const std::string& context, sqlite3* db) const {
		const int extended = db ? sqlite3_extended_errcode(db) : SQLITE_CANTOPEN;
		return {.operation = operation,
			.primary_code = extended & 0xff,
			.extended_code = extended,
			.path = path,
			.context = db ? context + ": " + sqlite3_errmsg(db) : context};
	}

}  // namespace gdl::cache
