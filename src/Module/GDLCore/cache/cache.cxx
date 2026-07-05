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


		class DownloadHistoryCache::Impl {
		   public:
			~Impl() {
				if (db_) {
					sqlite3_close(db_);
					db_ = nullptr;
				}
			}

			// Convert SQLite statement to DownloadRecord object
			DownloadRecord RecordFromStatement(sqlite3_stmt* stmt) {
				DownloadRecord record;
				record.task_id		   = ColumnText(stmt, 0);
				record.file_name	   = ColumnText(stmt, 1);
				record.save_path	   = ColumnText(stmt, 2);
				record.download_url	   = ColumnText(stmt, 3);
				record.total_size	   = sqlite3_column_int64(stmt, 4);
				record.downloaded_size = sqlite3_column_int64(stmt, 5);
				record.download_speed  = sqlite3_column_int(stmt, 6);
				record.connections	   = sqlite3_column_int(stmt, 7);
				record.state		   = static_cast<DownloadState>(sqlite3_column_int(stmt, 8));
				record.created_time	   = sqlite3_column_int64(stmt, 9);
				record.completed_time  = sqlite3_column_int64(stmt, 10);
				record.error_message   = ColumnText(stmt, 11);

				return record;
			}

			bool Initialize(const String& db_path) {
				if (db_) {
					return true;
				}

				auto dir = std::filesystem::path(db_path).parent_path();
				std::filesystem::create_directories(dir);

				int rc = sqlite3_open(db_path.c_str(), &db_);
				if (rc) {
					// 即使打开失败 db_ 也可能非 NULL，必须 close 并置空，
					// 否则后续 Initialize 会误判已初始化，所有操作静默失败。
					LOG_ERR("Can't open database: {}", sqlite3_errmsg(db_));
					sqlite3_close(db_);
					db_ = nullptr;
					return false;
				}

				return CreateTables();
			}

			bool CreateTables() {
				const char* sql = R"(
				CREATE TABLE IF NOT EXISTS download_history (
					task_id TEXT PRIMARY KEY,
					file_name TEXT NOT NULL,
					save_path TEXT NOT NULL,
					download_url TEXT NOT NULL,
					total_size INTEGER NOT NULL,
					downloaded_size INTEGER NOT NULL,
					download_speed INTEGER NOT NULL,
					connections INTEGER NOT NULL,
					state INTEGER NOT NULL,
					created_time INTEGER NOT NULL,
					completed_time INTEGER,
					error_message TEXT
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

			bool AddRecord(const DownloadRecord& record) {
				if (!db_) return false;

				const char* sql = R"(
				INSERT OR REPLACE INTO download_history (
					task_id, file_name, save_path, download_url,
					total_size, downloaded_size, download_speed, connections,
					state, created_time, completed_time, error_message
				) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
			)";

				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return false;
				}

				sqlite3_bind_text(stmt, 1, record.task_id.c_str(), -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt, 2, record.file_name.c_str(), -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt, 3, record.save_path.c_str(), -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt, 4, record.download_url.c_str(), -1, SQLITE_STATIC);
				sqlite3_bind_int64(stmt, 5, record.total_size);
				sqlite3_bind_int64(stmt, 6, record.downloaded_size);
				sqlite3_bind_int(stmt, 7, record.download_speed);
				sqlite3_bind_int(stmt, 8, record.connections);
				sqlite3_bind_int(stmt, 9, static_cast<int>(record.state));
				sqlite3_bind_int64(stmt, 10, record.created_time);
				sqlite3_bind_int64(stmt, 11, record.completed_time);
				sqlite3_bind_text(stmt, 12, record.error_message.c_str(), -1, SQLITE_STATIC);

				rc = sqlite3_step(stmt);
				sqlite3_finalize(stmt);

				return rc == SQLITE_DONE;
			}

			bool UpdateRecord(const DownloadRecord& record) {
				return AddRecord(record);  // 使用 INSERT OR REPLACE
			}

			bool DeleteRecord(const std::string& task_id) {
				if (!db_) return false;

				const char* sql = "DELETE FROM download_history WHERE task_id = ?";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return false;
				}

				sqlite3_bind_text(stmt, 1, task_id.c_str(), -1, SQLITE_STATIC);
				rc = sqlite3_step(stmt);
				sqlite3_finalize(stmt);

				return rc == SQLITE_DONE;
			}

			std::optional<DownloadRecord> GetRecord(const std::string& task_id) {
				if (!db_) return std::nullopt;

				const char* sql = "SELECT task_id, file_name, save_path, download_url, total_size, downloaded_size, download_speed, connections, state, created_time, completed_time, error_message FROM download_history WHERE task_id = ?";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return std::nullopt;
				}

				sqlite3_bind_text(stmt, 1, task_id.c_str(), -1, SQLITE_STATIC);

				std::optional<DownloadRecord> result;
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					result = RecordFromStatement(stmt);
				}

				sqlite3_finalize(stmt);
				return result;
			}

			std::vector<DownloadRecord> GetRecords(int limit, int offset) {
				std::vector<DownloadRecord> results;
				if (!db_) return results;

				std::string sql = "SELECT task_id, file_name, save_path, download_url, total_size, downloaded_size, download_speed, connections, state, created_time, completed_time, error_message FROM download_history ORDER BY created_time DESC";
				if (limit > 0) {
					sql += " LIMIT " + std::to_string(limit);
					if (offset > 0) {
						sql += " OFFSET " + std::to_string(offset);
					}
				}

				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return results;
				}

				while (sqlite3_step(stmt) == SQLITE_ROW) {
					results.push_back(RecordFromStatement(stmt));
				}

				sqlite3_finalize(stmt);
				return results;
			}

			std::vector<DownloadRecord> SearchByFileName(const std::string& keyword) {
				std::vector<DownloadRecord> results;
				if (!db_) return results;

				const char* sql = "SELECT task_id, file_name, save_path, download_url, total_size, downloaded_size, download_speed, connections, state, created_time, completed_time, error_message FROM download_history WHERE file_name LIKE ? ORDER BY created_time DESC";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return results;
				}

				std::string pattern = "%" + keyword + "%";
				sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_STATIC);

				while (sqlite3_step(stmt) == SQLITE_ROW) {
					results.push_back(RecordFromStatement(stmt));
				}

				sqlite3_finalize(stmt);
				return results;
			}

			std::vector<DownloadRecord> GetRecordsByState(DownloadState state) {
				std::vector<DownloadRecord> results;
				if (!db_) return results;

				const char* sql = "SELECT task_id, file_name, save_path, download_url, total_size, downloaded_size, download_speed, connections, state, created_time, completed_time, error_message FROM download_history WHERE state = ? ORDER BY created_time DESC";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return results;
				}

				sqlite3_bind_int(stmt, 1, static_cast<int>(state));

				while (sqlite3_step(stmt) == SQLITE_ROW) {
					results.push_back(RecordFromStatement(stmt));
				}

				sqlite3_finalize(stmt);
				return results;
			}

			std::vector<DownloadRecord> GetRecordsByDateRange(time_t start, time_t end) {
				std::vector<DownloadRecord> results;
				if (!db_) return results;

				const char* sql =
					"SELECT task_id, file_name, save_path, download_url, total_size, downloaded_size, download_speed, connections, state, created_time, completed_time, error_message FROM download_history WHERE created_time BETWEEN ? AND ? ORDER BY created_time DESC";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return results;
				}

				sqlite3_bind_int64(stmt, 1, start);
				sqlite3_bind_int64(stmt, 2, end);

				while (sqlite3_step(stmt) == SQLITE_ROW) {
					results.push_back(RecordFromStatement(stmt));
				}

				sqlite3_finalize(stmt);
				return results;
			}

			size_t GetTotalCount() {
				if (!db_) return 0;

				const char* sql = "SELECT COUNT(*) FROM download_history";
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

			size_t GetCountByState(DownloadState state) {
				if (!db_) return 0;

				const char* sql = "SELECT COUNT(*) FROM download_history WHERE state = ?";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return 0;
				}

				sqlite3_bind_int(stmt, 1, static_cast<int>(state));

				size_t count = 0;
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
				}

				sqlite3_finalize(stmt);
				return count;
			}

			int64_t GetTotalDownloadSize() {
				if (!db_) return 0;

				const char* sql = "SELECT SUM(downloaded_size) FROM download_history";
				sqlite3_stmt* stmt;
				int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
				if (rc != SQLITE_OK) {
					LOG_ERR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
					return 0;
				}

				int64_t total_size = 0;
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					total_size = sqlite3_column_int64(stmt, 0);
				}

				sqlite3_finalize(stmt);
				return total_size;
			}

		   private:
			sqlite3* db_{nullptr};
			static constexpr const char* kTableName = "download_history";
		};

		DownloadHistoryCache::DownloadHistoryCache() : impl_(std::make_unique<Impl>()) {}
		DownloadHistoryCache::~DownloadHistoryCache() = default;

		bool DownloadHistoryCache::Initialize(const String& db_path) {
			return impl_->Initialize(db_path);
		}

		void DownloadHistoryCache::Uninitialize() {
			impl_.reset();
		}

		bool DownloadHistoryCache::AddRecord(const DownloadRecord& record) {
			return impl_->AddRecord(record);
		}

		bool DownloadHistoryCache::UpdateRecord(const DownloadRecord& record) {
			return impl_->UpdateRecord(record);
		}

		bool DownloadHistoryCache::DeleteRecord(const std::string& task_id) {
			return impl_->DeleteRecord(task_id);
		}

		std::optional<DownloadRecord> DownloadHistoryCache::GetRecord(const std::string& task_id) {
			return impl_->GetRecord(task_id);
		}

		std::vector<DownloadRecord> DownloadHistoryCache::GetRecords(int limit, int offset) {
			return impl_->GetRecords(limit, offset);
		}

		std::vector<DownloadRecord> DownloadHistoryCache::SearchByFileName(const std::string& keyword) {
			return impl_->SearchByFileName(keyword);
		}

		std::vector<DownloadRecord> DownloadHistoryCache::GetRecordsByState(DownloadState state) {
			return impl_->GetRecordsByState(state);
		}

		std::vector<DownloadRecord> DownloadHistoryCache::GetRecordsByDateRange(time_t start, time_t end) {
			return impl_->GetRecordsByDateRange(start, end);
		}

		size_t DownloadHistoryCache::GetTotalCount() {
			return impl_->GetTotalCount();
		}

		size_t DownloadHistoryCache::GetCountByState(DownloadState state) {
			return impl_->GetCountByState(state);
		}

		int64_t DownloadHistoryCache::GetTotalDownloadSize() {
			return impl_->GetTotalDownloadSize();
		}

		bool DownloadHistoryCache::ClearRecords() {
			return impl_->ExecuteSQL("DELETE FROM download_history");
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
