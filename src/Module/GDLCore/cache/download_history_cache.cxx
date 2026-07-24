#include "cache.h"

#include <sqlite3.h>

#include <memory>
#include <string>
#include <vector>

#include "schema_migrator.h"
#include "sqlite_connection.h"

namespace gdl::cache {
	namespace {
		constexpr const char* kColumns =
			"task_id,file_name,save_path,download_url,total_size,downloaded_size,download_speed,"
			"connections,state,created_time,completed_time,error_message";

		struct StatementDeleter { void operator()(sqlite3_stmt* stmt) const { sqlite3_finalize(stmt); } };
		using Statement = std::unique_ptr<sqlite3_stmt, StatementDeleter>;

		CacheError SqlError(sqlite3* db, CacheOperation operation, const std::string& path,
			const std::string& context) {
			const int extended = sqlite3_extended_errcode(db);
			return {.operation = operation, .primary_code = extended & 0xff,
				.extended_code = extended, .path = path, .context = context + ": " + sqlite3_errmsg(db)};
		}

		CacheResult<Statement> Prepare(sqlite3* db, const std::string& path, const char* sql) {
			sqlite3_stmt* raw = nullptr;
			if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK)
				return CacheResult<Statement>::Failure(SqlError(db, CacheOperation::kPrepare, path, sql));
			return CacheResult<Statement>::Success(Statement(raw));
		}

		CacheResult<void> BindText(sqlite3* db, sqlite3_stmt* stmt, int index,
			const std::string& value, const std::string& path) {
			if (sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
				return CacheResult<void>::Failure(SqlError(db, CacheOperation::kBind, path, "bind text"));
			return CacheResult<void>::Success();
		}

		std::string Text(sqlite3_stmt* stmt, int column) {
			const auto* value = sqlite3_column_text(stmt, column);
			return value ? reinterpret_cast<const char*>(value) : std::string{};
		}

		DownloadRecord ReadRecord(sqlite3_stmt* stmt) {
			return {.task_id = Text(stmt, 0), .file_name = Text(stmt, 1), .save_path = Text(stmt, 2),
				.download_url = Text(stmt, 3), .total_size = sqlite3_column_int64(stmt, 4),
				.downloaded_size = sqlite3_column_int64(stmt, 5), .download_speed = sqlite3_column_int(stmt, 6),
				.connections = sqlite3_column_int(stmt, 7), .state = static_cast<DownloadState>(sqlite3_column_int(stmt, 8)),
				.created_time = sqlite3_column_int64(stmt, 9), .completed_time = sqlite3_column_int64(stmt, 10),
				.error_message = Text(stmt, 11)};
		}

		SchemaDefinition HistorySchema() {
			return {.table_name = "download_history", .current_version = 1,
				.create_table_sql = "CREATE TABLE download_history(task_id TEXT PRIMARY KEY,file_name TEXT NOT NULL,save_path TEXT NOT NULL,download_url TEXT NOT NULL,total_size INTEGER NOT NULL,downloaded_size INTEGER NOT NULL,download_speed INTEGER NOT NULL,connections INTEGER NOT NULL,state INTEGER NOT NULL,created_time INTEGER NOT NULL,completed_time INTEGER,error_message TEXT)",
				.current_columns = {{"task_id","TEXT",false,"",1},{"file_name","TEXT",true,"",0},{"save_path","TEXT",true,"",0},{"download_url","TEXT",true,"",0},{"total_size","INTEGER",true,"",0},{"downloaded_size","INTEGER",true,"",0},{"download_speed","INTEGER",true,"",0},{"connections","INTEGER",true,"",0},{"state","INTEGER",true,"",0},{"created_time","INTEGER",true,"",0},{"completed_time","INTEGER",false,"",0},{"error_message","TEXT",false,"",0}}};
		}
	}

	class DownloadHistoryCache::Impl {
	 public:
		CacheResult<void> Initialize(const String& path) {
			// 历史库可能在升级、退出或杀毒软件扫描期间短暂被其他实例占用；
			// 250ms 在 Windows 上过短，会把可恢复的 SQLITE_BUSY 暴露为删除失败。
			auto opened = connection_.Open(path, {.busy_timeout_ms = 3000, .request_wal = true});
			if (opened.HasError()) return opened;
			auto schema = EnsureSchema(connection_, HistorySchema());
			if (schema.HasError()) { connection_.Close(); return schema; }
			return CacheResult<void>::Success();
		}
		CacheResult<void> Close() { return connection_.Close(); }
		SqliteConnection& Connection() { return connection_; }
	 private: SqliteConnection connection_;
	};

	DownloadHistoryCache::DownloadHistoryCache() : impl_(std::make_unique<Impl>()) {}
	DownloadHistoryCache::~DownloadHistoryCache() = default;
	CacheResult<void> DownloadHistoryCache::Initialize(const String& path) { return impl_->Initialize(path); }
	CacheResult<void> DownloadHistoryCache::Uninitialize() { return impl_->Close(); }

	CacheResult<void> DownloadHistoryCache::AddRecord(const DownloadRecord& r) {
		return impl_->Connection().Use<void>(CacheOperation::kStep, "upsert download history", [&](sqlite3* db, const std::string& database_path) {
			auto prepared = Prepare(db, database_path, "INSERT INTO download_history(task_id,file_name,save_path,download_url,total_size,downloaded_size,download_speed,connections,state,created_time,completed_time,error_message) VALUES(?,?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(task_id) DO UPDATE SET file_name=excluded.file_name,save_path=excluded.save_path,download_url=excluded.download_url,total_size=excluded.total_size,downloaded_size=excluded.downloaded_size,download_speed=excluded.download_speed,connections=excluded.connections,state=excluded.state,created_time=excluded.created_time,completed_time=excluded.completed_time,error_message=excluded.error_message");
			if (prepared.HasError()) return CacheResult<void>::Failure(prepared.GetError()); auto& s=prepared.Value();
			const std::string* texts[]={&r.task_id,&r.file_name,&r.save_path,&r.download_url}; for(int i=0;i<4;++i){auto b=BindText(db,s.get(),i+1,*texts[i],database_path);if(b.HasError())return b;}
			int rc=SQLITE_OK; rc=sqlite3_bind_int64(s.get(),5,r.total_size); if(rc==SQLITE_OK)rc=sqlite3_bind_int64(s.get(),6,r.downloaded_size); if(rc==SQLITE_OK)rc=sqlite3_bind_int(s.get(),7,r.download_speed); if(rc==SQLITE_OK)rc=sqlite3_bind_int(s.get(),8,r.connections); if(rc==SQLITE_OK)rc=sqlite3_bind_int(s.get(),9,static_cast<int>(r.state)); if(rc==SQLITE_OK)rc=sqlite3_bind_int64(s.get(),10,r.created_time); if(rc==SQLITE_OK)rc=sqlite3_bind_int64(s.get(),11,r.completed_time); if(rc!=SQLITE_OK)return CacheResult<void>::Failure(SqlError(db,CacheOperation::kBind,database_path,"bind record")); auto b=BindText(db,s.get(),12,r.error_message,database_path);if(b.HasError())return b;
			if(sqlite3_step(s.get())!=SQLITE_DONE)return CacheResult<void>::Failure(SqlError(db,CacheOperation::kStep,database_path,"upsert record"));return CacheResult<void>::Success(); });
	}
	CacheResult<void> DownloadHistoryCache::UpdateRecord(const DownloadRecord& r){return AddRecord(r);}
	CacheResult<void> DownloadHistoryCache::DeleteRecord(const std::string& id){return impl_->Connection().Use<void>(CacheOperation::kStep,"delete record",[&](sqlite3*db, const std::string& database_path){auto p=Prepare(db,database_path,"DELETE FROM download_history WHERE task_id=?");if(p.HasError())return CacheResult<void>::Failure(p.GetError());auto b=BindText(db,p.Value().get(),1,id,database_path);if(b.HasError())return b;if(sqlite3_step(p.Value().get())!=SQLITE_DONE)return CacheResult<void>::Failure(SqlError(db,CacheOperation::kStep,database_path,"delete"));return CacheResult<void>::Success();});}
	CacheResult<std::optional<DownloadRecord>> DownloadHistoryCache::GetRecord(const std::string&id){return impl_->Connection().Use<std::optional<DownloadRecord>>(CacheOperation::kInspect,"get record",[&](sqlite3*db, const std::string& database_path){std::string sql="SELECT "+std::string(kColumns)+" FROM download_history WHERE task_id=?";auto p=Prepare(db,database_path,sql.c_str());if(p.HasError())return CacheResult<std::optional<DownloadRecord>>::Failure(p.GetError());auto b=BindText(db,p.Value().get(),1,id,database_path);if(b.HasError())return CacheResult<std::optional<DownloadRecord>>::Failure(b.GetError());int rc=sqlite3_step(p.Value().get());if(rc==SQLITE_ROW)return CacheResult<std::optional<DownloadRecord>>::Success(std::optional<DownloadRecord>(ReadRecord(p.Value().get())));if(rc==SQLITE_DONE)return CacheResult<std::optional<DownloadRecord>>::Success(std::nullopt);return CacheResult<std::optional<DownloadRecord>>::Failure(SqlError(db,CacheOperation::kStep,database_path,"select record"));});}

	static CacheResult<std::vector<DownloadRecord>> Query(DownloadHistoryCache::Impl* impl,const std::string&sql){return impl->Connection().Use<std::vector<DownloadRecord>>(CacheOperation::kInspect,"query records",[&](sqlite3*db, const std::string& database_path){auto p=Prepare(db,database_path,sql.c_str());if(p.HasError())return CacheResult<std::vector<DownloadRecord>>::Failure(p.GetError());std::vector<DownloadRecord> out;for(;;){int rc=sqlite3_step(p.Value().get());if(rc==SQLITE_ROW)out.push_back(ReadRecord(p.Value().get()));else if(rc==SQLITE_DONE)return CacheResult<std::vector<DownloadRecord>>::Success(std::move(out));else return CacheResult<std::vector<DownloadRecord>>::Failure(SqlError(db,CacheOperation::kStep,database_path,"query records"));}});}
	template <typename Binder>
	static CacheResult<std::vector<DownloadRecord>> QueryBound(DownloadHistoryCache::Impl* impl,const std::string&sql,Binder binder){return impl->Connection().Use<std::vector<DownloadRecord>>(CacheOperation::kInspect,"query records",[&](sqlite3*db, const std::string& database_path){auto p=Prepare(db,database_path,sql.c_str());if(p.HasError())return CacheResult<std::vector<DownloadRecord>>::Failure(p.GetError());auto bound=binder(db,p.Value().get(),database_path);if(bound.HasError())return CacheResult<std::vector<DownloadRecord>>::Failure(bound.GetError());std::vector<DownloadRecord> out;for(;;){int rc=sqlite3_step(p.Value().get());if(rc==SQLITE_ROW)out.push_back(ReadRecord(p.Value().get()));else if(rc==SQLITE_DONE)return CacheResult<std::vector<DownloadRecord>>::Success(std::move(out));else return CacheResult<std::vector<DownloadRecord>>::Failure(SqlError(db,CacheOperation::kStep,database_path,"query records"));}});}
	CacheResult<std::vector<DownloadRecord>> DownloadHistoryCache::GetRecords(int limit,int offset){std::string q="SELECT "+std::string(kColumns)+" FROM download_history ORDER BY created_time DESC";if(limit>0)q+=" LIMIT "+std::to_string(limit)+" OFFSET "+std::to_string(std::max(0,offset));return Query(impl_.get(),q);}
	CacheResult<std::vector<DownloadRecord>> DownloadHistoryCache::SearchByFileName(const std::string& keyword){std::string pattern="%"+keyword+"%";return QueryBound(impl_.get(),"SELECT "+std::string(kColumns)+" FROM download_history WHERE file_name LIKE ? ORDER BY created_time DESC",[&](sqlite3*db,sqlite3_stmt*s,const std::string& database_path){return BindText(db,s,1,pattern,database_path);});}
	CacheResult<std::vector<DownloadRecord>> DownloadHistoryCache::GetRecordsByState(DownloadState state){return QueryBound(impl_.get(),"SELECT "+std::string(kColumns)+" FROM download_history WHERE state=? ORDER BY created_time DESC",[&](sqlite3*db,sqlite3_stmt*s,const std::string& database_path){if(sqlite3_bind_int(s,1,static_cast<int>(state))!=SQLITE_OK)return CacheResult<void>::Failure(SqlError(db,CacheOperation::kBind,database_path,"bind state"));return CacheResult<void>::Success();});}
	CacheResult<std::vector<DownloadRecord>> DownloadHistoryCache::GetRecordsByDateRange(time_t start,time_t end){return QueryBound(impl_.get(),"SELECT "+std::string(kColumns)+" FROM download_history WHERE created_time BETWEEN ? AND ? ORDER BY created_time DESC",[&](sqlite3*db,sqlite3_stmt*s,const std::string& database_path){if(sqlite3_bind_int64(s,1,start)!=SQLITE_OK||sqlite3_bind_int64(s,2,end)!=SQLITE_OK)return CacheResult<void>::Failure(SqlError(db,CacheOperation::kBind,database_path,"bind date range"));return CacheResult<void>::Success();});}
	CacheResult<size_t> DownloadHistoryCache::GetTotalCount(){return impl_->Connection().Use<size_t>(CacheOperation::kInspect,"count",[&](sqlite3*db, const std::string& database_path){auto p=Prepare(db,database_path,"SELECT count(*) FROM download_history");if(p.HasError())return CacheResult<size_t>::Failure(p.GetError());if(sqlite3_step(p.Value().get())!=SQLITE_ROW)return CacheResult<size_t>::Failure(SqlError(db,CacheOperation::kStep,database_path,"count"));return CacheResult<size_t>::Success(static_cast<size_t>(sqlite3_column_int64(p.Value().get(),0)));});}
	CacheResult<size_t> DownloadHistoryCache::GetCountByState(DownloadState state){return impl_->Connection().Use<size_t>(CacheOperation::kInspect,"count state",[&](sqlite3*db, const std::string& database_path){auto p=Prepare(db,database_path,"SELECT count(*) FROM download_history WHERE state=?");if(p.HasError())return CacheResult<size_t>::Failure(p.GetError());if(sqlite3_bind_int(p.Value().get(),1,static_cast<int>(state))!=SQLITE_OK)return CacheResult<size_t>::Failure(SqlError(db,CacheOperation::kBind,database_path,"bind state"));if(sqlite3_step(p.Value().get())!=SQLITE_ROW)return CacheResult<size_t>::Failure(SqlError(db,CacheOperation::kStep,database_path,"count state"));return CacheResult<size_t>::Success(static_cast<size_t>(sqlite3_column_int64(p.Value().get(),0)));});}
	CacheResult<int64_t> DownloadHistoryCache::GetTotalDownloadSize(){return impl_->Connection().Use<int64_t>(CacheOperation::kInspect,"sum",[&](sqlite3*db, const std::string& database_path){auto p=Prepare(db,database_path,"SELECT COALESCE(sum(downloaded_size),0) FROM download_history");if(p.HasError())return CacheResult<int64_t>::Failure(p.GetError());if(sqlite3_step(p.Value().get())!=SQLITE_ROW)return CacheResult<int64_t>::Failure(SqlError(db,CacheOperation::kStep,database_path,"sum"));return CacheResult<int64_t>::Success(sqlite3_column_int64(p.Value().get(),0));});}
	CacheResult<void> DownloadHistoryCache::ClearRecords(){return impl_->Connection().Use<void>(CacheOperation::kStep,"clear",[&](sqlite3*db, const std::string& database_path){if(sqlite3_exec(db,"DELETE FROM download_history",nullptr,nullptr,nullptr)!=SQLITE_OK)return CacheResult<void>::Failure(SqlError(db,CacheOperation::kStep,database_path,"clear"));return CacheResult<void>::Success();});}
}

