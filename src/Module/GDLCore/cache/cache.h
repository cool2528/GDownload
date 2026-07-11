#pragma once
#include <memory>
#include <optional>
#include <vector>
#include "download_record.h"
#include "cache_result.h"
#include "sqlite_connection.h"
#include "export.h"
#include "globalTypes.h"
#include "singleton.hpp"
namespace gdl {
	namespace cache {
		namespace detail {
			constexpr SqliteOpenOptions TrackerCacheOpenOptions() { return {.busy_timeout_ms = 3000, .request_wal = true}; }
		}

        class GDLCore_API DownloadHistoryCache : public Singleton<DownloadHistoryCache> {
            SINGLETON_DECLARE(DownloadHistoryCache)
		   public:
			CacheResult<void> Initialize(const String& db_path);
			CacheResult<void> Uninitialize();
            ~DownloadHistoryCache();
			// 基础CRUD操作
			CacheResult<void> AddRecord(const DownloadRecord& record);
			CacheResult<void> UpdateRecord(const DownloadRecord& record);
			CacheResult<void> DeleteRecord(const std::string& task_id);
			CacheResult<std::optional<DownloadRecord>> GetRecord(const std::string& task_id);
			CacheResult<std::vector<DownloadRecord>> GetRecords(int limit = -1, int offset = 0);

			// 查询接口
			CacheResult<std::vector<DownloadRecord>> SearchByFileName(const std::string& keyword);
			CacheResult<std::vector<DownloadRecord>> GetRecordsByState(DownloadState state);
			CacheResult<std::vector<DownloadRecord>> GetRecordsByDateRange(time_t start, time_t end);

			// 统计接口
			CacheResult<size_t> GetTotalCount();
			CacheResult<size_t> GetCountByState(DownloadState state);
			CacheResult<int64_t> GetTotalDownloadSize();

			CacheResult<void> ClearRecords();
			class Impl;

		   private:
			DownloadHistoryCache();
			std::unique_ptr<Impl> impl_;
		};

		// Tracker ETag 缓存结构
		struct TrackerETagEntry {
			std::string url;		// Tracker 源 URL
			std::string etag;		// HTTP ETag
			std::string content;	// 缓存内容
			std::int64_t timestamp;	// 更新时间戳
		};

		class GDLCore_API TrackerETagCache : public Singleton<TrackerETagCache> {
			SINGLETON_DECLARE(TrackerETagCache)
		   public:
			CacheResult<void> Initialize(const String& db_path);
			CacheResult<void> Uninitialize();
			~TrackerETagCache();

			// CRUD 操作
			CacheResult<void> SetEntry(const TrackerETagEntry& entry);
			CacheResult<std::optional<TrackerETagEntry>> GetEntry(const std::string& url);
			CacheResult<void> DeleteEntry(const std::string& url);
			CacheResult<void> ClearAllEntries();

			// 批量操作
			CacheResult<std::vector<TrackerETagEntry>> GetAllEntries();
			CacheResult<size_t> GetTotalCount();

		   private:
			TrackerETagCache();
			class Impl;
			std::unique_ptr<Impl> impl_;
		};

	}  // namespace cache
}  // namespace gdl
