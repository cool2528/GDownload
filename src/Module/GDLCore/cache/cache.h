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

	}  // namespace cache
}  // namespace gdl
