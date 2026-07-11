#pragma once

#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

#include "cache_result.h"
#include "export.h"

struct sqlite3;

namespace gdl::cache {

	struct SqliteOpenOptions {
		int busy_timeout_ms{5000};
		bool request_wal{true};
	};

	class GDLCore_API SqliteConnection {
	   public:
		SqliteConnection() = default;
		~SqliteConnection();

		SqliteConnection(const SqliteConnection&) = delete;
		SqliteConnection& operator=(const SqliteConnection&) = delete;

		CacheResult<void> Open(const std::string& path, SqliteOpenOptions options = {});
		CacheResult<void> Close();
		[[nodiscard]] bool IsOpen() const;
		[[nodiscard]] std::string Path() const;

		template <typename T, typename Callback>
		CacheResult<T> Use(CacheOperation operation, std::string context, Callback&& callback) {
			ThrowIfReentrant();
			std::lock_guard lock(mutex_);
			if (!db_) {
				return CacheResult<T>::Failure(MakeClosedError(operation, std::move(context)));
			}
			UseGuard guard(this);
			return std::forward<Callback>(callback)(db_);
		}

	   private:
		class UseGuard {
		   public:
			explicit UseGuard(const SqliteConnection* connection) : connection_(connection) {
				connection_->EnterUse();
			}
			~UseGuard() { connection_->LeaveUse(); }

		   private:
			const SqliteConnection* connection_;
		};

		void ThrowIfReentrant() const;
		void EnterUse() const;
		void LeaveUse() const;
		CacheError MakeClosedError(CacheOperation operation, std::string context) const;
		CacheError MakeError(CacheOperation operation, const std::string& path,
			const std::string& context, sqlite3* db) const;
		CacheResult<void> CloseUnlocked();

		mutable std::mutex mutex_;
		sqlite3* db_{nullptr};
		std::string path_;
	};

}  // namespace gdl::cache
