#pragma once
#include <memory>
#include <optional>
#include <vector>
#include "download_record.h"
#include "export.h"
#include "globalTypes.h"
#include "singleton.hpp"
namespace gdl {
	namespace cache {

        class GDLCore_API DownloadHistoryCache : public Singleton<DownloadHistoryCache> {
            SINGLETON_DECLARE(DownloadHistoryCache)
		   public:
			bool Initialize(const String& db_path);
			void Uninitialize();
            ~DownloadHistoryCache();
			// 基础CRUD操作
			bool AddRecord(const DownloadRecord& record);
			bool UpdateRecord(const DownloadRecord& record);
			bool DeleteRecord(const std::string& task_id);
			std::optional<DownloadRecord> GetRecord(const std::string& task_id);
			std::vector<DownloadRecord> GetRecords(int limit = -1, int offset = 0);

			// 查询接口
			std::vector<DownloadRecord> SearchByFileName(const std::string& keyword);
			std::vector<DownloadRecord> GetRecordsByState(DownloadState state);
			std::vector<DownloadRecord> GetRecordsByDateRange(time_t start, time_t end);

			// 统计接口
			size_t GetTotalCount();
			size_t GetCountByState(DownloadState state);
			int64_t GetTotalDownloadSize();

			bool ClearRecords();

		   private:
			DownloadHistoryCache();
			class Impl;
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
			bool Initialize(const String& db_path);
			void Uninitialize();
			~TrackerETagCache();

			// CRUD 操作
			bool SetEntry(const TrackerETagEntry& entry);
			std::optional<TrackerETagEntry> GetEntry(const std::string& url);
			bool DeleteEntry(const std::string& url);
			bool ClearAllEntries();

			// 批量操作
			std::vector<TrackerETagEntry> GetAllEntries();
			size_t GetTotalCount();

		   private:
			TrackerETagCache();
			class Impl;
			std::unique_ptr<Impl> impl_;
		};

	}  // namespace cache
}  // namespace gdl
