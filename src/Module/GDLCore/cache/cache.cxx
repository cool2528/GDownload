#include "cache.h"
#include <sqlite3.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "logger.h"

namespace gdl {
	namespace cache {

		// 安全读取 SQLite 文本列：列值为 NULL 时返回空串，避免用 nullptr 构造 std::string 的 UB。
		inline std::string ColumnText(sqlite3_stmt* stmt, int col) {
			const auto* p = sqlite3_column_text(stmt, col);
			return p ? reinterpret_cast<const char*>(p) : std::string{};
		}



		// ==================== TrackerETagCache 实现 ====================

		class TrackerETagCache::Impl {
		   public:
			~Impl() {
				if (db_) {
					sqlite3_close(db_);
					db_ = nullptr;
				}
			}

			bool Initialize(const String& db_path) {
				if (db_) {
					return true;
				}

				auto dir = std::filesystem::path(db_path).parent_path();
				std::filesystem::create_directories(dir);

				int rc = sqlite3_open(db_path.c_str(), &db_);
				if (rc) {
					// 即使打开失败 db_ 也可能非 NULL，必须 close 并置空。
					LOG_ERR("Can't open database: {}", sqlite3_errmsg(db_));
					sqlite3_close(db_);
					db_ = nullptr;
					return false;
				}

				return CreateTables();
			}

			bool CreateTables() {
				const char* sql = R"(
				CREATE TABLE IF NOT EXISTS tracker_etag_cache (
					url TEXT PRIMARY KEY,
					etag TEXT NOT NULL,
					content TEXT NOT NULL,
					timestamp INTEGER NOT NULL
				)
			)";

				return ExecuteSQL(sql);
			}

			bool ExecuteSQL(const char* sql) {
				if (!db_) return false;

				char* err_msg = nullptr;
				int rc		  = sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg);

				if (rc != SQLITE_OK) {
					LOG_ERR("SQL error: {}", err_msg);
					sqlite3_free(err_msg);
					return false;
				}

				return true;
			}

			TrackerETagEntry EntryFromStatement(sqlite3_stmt* stmt) {
				TrackerETagEntry entry;
				entry.url = ColumnText(stmt, 0);
				entry.etag = ColumnText(stmt, 1);
				entry.content = ColumnText(stmt, 2);
				entry.timestamp = sqlite3_column_int64(stmt, 3);
				return entry;
			}

			bool SetEntry(const TrackerETagEntry& entry) {
				if (!db_) return false;

				const char* sql = R"(
				INSERT OR REPLACE INTO tracker_etag_cache (url, etag, content, timestamp)
				VALUES (?, ?, ?, ?)
			)";

				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return false;
				}

				sqlite3_bind_text(stmt, 1, entry.url.c_str(), -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt, 2, entry.etag.c_str(), -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt, 3, entry.content.c_str(), -1, SQLITE_STATIC);
				sqlite3_bind_int64(stmt, 4, entry.timestamp);

				rc = sqlite3_step(stmt);
				sqlite3_finalize(stmt);

				return rc == SQLITE_DONE;
			}

			std::optional<TrackerETagEntry> GetEntry(const std::string& url) {
				if (!db_) return std::nullopt;

				const char* sql = "SELECT url, etag, content, timestamp FROM tracker_etag_cache WHERE url = ?";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return std::nullopt;
				}

				sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);

				std::optional<TrackerETagEntry> result;
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					result = EntryFromStatement(stmt);
				}

				sqlite3_finalize(stmt);
				return result;
			}

			bool DeleteEntry(const std::string& url) {
				if (!db_) return false;

				const char* sql = "DELETE FROM tracker_etag_cache WHERE url = ?";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return false;
				}

				sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
				rc = sqlite3_step(stmt);
				sqlite3_finalize(stmt);

				return rc == SQLITE_DONE;
			}

			bool ClearAllEntries() {
				return ExecuteSQL("DELETE FROM tracker_etag_cache");
			}

			std::vector<TrackerETagEntry> GetAllEntries() {
				std::vector<TrackerETagEntry> results;
				if (!db_) return results;

				const char* sql = "SELECT url, etag, content, timestamp FROM tracker_etag_cache ORDER BY timestamp DESC";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return results;
				}

				while (sqlite3_step(stmt) == SQLITE_ROW) {
					results.push_back(EntryFromStatement(stmt));
				}

				sqlite3_finalize(stmt);
				return results;
			}

			size_t GetTotalCount() {
				if (!db_) return 0;

				const char* sql = "SELECT COUNT(*) FROM tracker_etag_cache";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return 0;
				}

				size_t count = 0;
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
				}

				sqlite3_finalize(stmt);
				return count;
			}

		   private:
			sqlite3* db_{nullptr};
		};

		TrackerETagCache::TrackerETagCache() : impl_(std::make_unique<Impl>()) {}
		TrackerETagCache::~TrackerETagCache() = default;

		bool TrackerETagCache::Initialize(const String& db_path) {
			return impl_->Initialize(db_path);
		}

		void TrackerETagCache::Uninitialize() {
			impl_.reset();
		}

		bool TrackerETagCache::SetEntry(const TrackerETagEntry& entry) {
			return impl_->SetEntry(entry);
		}

		std::optional<TrackerETagEntry> TrackerETagCache::GetEntry(const std::string& url) {
			return impl_->GetEntry(url);
		}

		bool TrackerETagCache::DeleteEntry(const std::string& url) {
			return impl_->DeleteEntry(url);
		}

		bool TrackerETagCache::ClearAllEntries() {
			return impl_->ClearAllEntries();
		}

		std::vector<TrackerETagEntry> TrackerETagCache::GetAllEntries() {
			return impl_->GetAllEntries();
		}

		size_t TrackerETagCache::GetTotalCount() {
			return impl_->GetTotalCount();
		}

	}  // namespace cache
}  // namespace gdl
