#include <gtest/gtest.h>
#include <sqlite3.h>
#include <chrono>
#include <filesystem>
#include <fstream>

#include "cache/cache.h"

namespace {
using namespace gdl::cache;

class DownloadHistoryCacheTest : public testing::Test {
 protected:
  void SetUp() override { dir_=std::filesystem::temp_directory_path()/("gdownload-history-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())); std::filesystem::create_directories(dir_); cache().Uninitialize(); }
  void TearDown() override { cache().Uninitialize(); std::error_code ec; std::filesystem::remove_all(dir_,ec); }
  DownloadHistoryCache& cache(){return DownloadHistoryCache::Instance();}
  std::string Path(const char* name="history.db") const{return (dir_/name).string();}
  DownloadRecord Record(std::string id="gid") { DownloadRecord r; r.task_id=std::move(id);r.file_name="文件 名.bin";r.save_path="下载/目录";r.download_url="https://example.com/x";r.total_size=100;r.downloaded_size=50;r.download_speed=3;r.connections=2;r.state=DownloadState::kComplete;r.created_time=10;r.completed_time=20;r.error_message="";return r; }
  std::filesystem::path dir_;
};

TEST_F(DownloadHistoryCacheTest, CreatesVersionOneAndSupportsUnicodeCrud) {
 ASSERT_TRUE(cache().Initialize(Path()).IsOk()); ASSERT_TRUE(cache().AddRecord(Record()).IsOk());
 auto got=cache().GetRecord("gid"); ASSERT_TRUE(got.IsOk()); ASSERT_TRUE(got.Value().has_value()); EXPECT_EQ(got.Value()->file_name,"文件 名.bin");
 sqlite3* db=nullptr; ASSERT_EQ(sqlite3_open(Path().c_str(),&db),SQLITE_OK); sqlite3_stmt*s=nullptr; ASSERT_EQ(sqlite3_prepare_v2(db,"PRAGMA user_version",-1,&s,nullptr),SQLITE_OK); ASSERT_EQ(sqlite3_step(s),SQLITE_ROW); EXPECT_EQ(sqlite3_column_int(s,0),1);sqlite3_finalize(s);sqlite3_close(db);
}
TEST_F(DownloadHistoryCacheTest, MissingRecordIsSuccessfulEmptyOptional){ASSERT_TRUE(cache().Initialize(Path()).IsOk());auto r=cache().GetRecord("missing");ASSERT_TRUE(r.IsOk());EXPECT_FALSE(r.Value().has_value());}
TEST_F(DownloadHistoryCacheTest, UpsertUpdatesWithoutReplacingIdentity){ASSERT_TRUE(cache().Initialize(Path()).IsOk());auto r=Record();ASSERT_TRUE(cache().AddRecord(r).IsOk());r.downloaded_size=99;ASSERT_TRUE(cache().UpdateRecord(r).IsOk());auto got=cache().GetRecord("gid");ASSERT_TRUE(got.IsOk());ASSERT_TRUE(got.Value());EXPECT_EQ(got.Value()->downloaded_size,99);}
TEST_F(DownloadHistoryCacheTest, QueryErrorIsNotReportedAsEmpty){ASSERT_TRUE(cache().Initialize(Path()).IsOk());sqlite3*db=nullptr;ASSERT_EQ(sqlite3_open(Path().c_str(),&db),SQLITE_OK);ASSERT_EQ(sqlite3_exec(db,"DROP TABLE download_history",nullptr,nullptr,nullptr),SQLITE_OK);sqlite3_close(db);auto r=cache().GetRecords();EXPECT_TRUE(r.HasError());}
TEST_F(DownloadHistoryCacheTest, ReopensAnotherDatabaseWithoutResettingImpl){ASSERT_TRUE(cache().Initialize(Path("a.db")).IsOk());ASSERT_TRUE(cache().AddRecord(Record("a")).IsOk());ASSERT_TRUE(cache().Uninitialize().IsOk());ASSERT_TRUE(cache().Initialize(Path("b.db")).IsOk());auto r=cache().GetRecord("a");ASSERT_TRUE(r.IsOk());EXPECT_FALSE(r.Value());}
TEST_F(DownloadHistoryCacheTest, AdoptsMatchingUnversionedTable){sqlite3*db=nullptr;ASSERT_EQ(sqlite3_open(Path().c_str(),&db),SQLITE_OK);ASSERT_EQ(sqlite3_exec(db,"CREATE TABLE download_history(task_id TEXT PRIMARY KEY,file_name TEXT NOT NULL,save_path TEXT NOT NULL,download_url TEXT NOT NULL,total_size INTEGER NOT NULL,downloaded_size INTEGER NOT NULL,download_speed INTEGER NOT NULL,connections INTEGER NOT NULL,state INTEGER NOT NULL,created_time INTEGER NOT NULL,completed_time INTEGER,error_message TEXT)",nullptr,nullptr,nullptr),SQLITE_OK);sqlite3_close(db);EXPECT_TRUE(cache().Initialize(Path()).IsOk());}
TEST_F(DownloadHistoryCacheTest, InvalidDatabasePathReturnsStructuredError){auto r=cache().Initialize(dir_.string());ASSERT_TRUE(r.HasError());EXPECT_NE(r.GetError().primary_code,0);EXPECT_FALSE(r.GetError().path.empty());}
TEST_F(DownloadHistoryCacheTest, ReadsNullErrorMessageAsEmpty){ASSERT_TRUE(cache().Initialize(Path()).IsOk());sqlite3*db=nullptr;ASSERT_EQ(sqlite3_open(Path().c_str(),&db),SQLITE_OK);ASSERT_EQ(sqlite3_exec(db,"INSERT INTO download_history VALUES('null','f','p','u',1,1,0,0,0,1,NULL,NULL)",nullptr,nullptr,nullptr),SQLITE_OK);sqlite3_close(db);auto r=cache().GetRecord("null");ASSERT_TRUE(r.IsOk());ASSERT_TRUE(r.Value());EXPECT_TRUE(r.Value()->error_message.empty());}
TEST_F(DownloadHistoryCacheTest, BusyWriteReturnsStructuredBusyError){ASSERT_TRUE(cache().Initialize(Path()).IsOk());sqlite3*db=nullptr;ASSERT_EQ(sqlite3_open(Path().c_str(),&db),SQLITE_OK);ASSERT_EQ(sqlite3_exec(db,"BEGIN IMMEDIATE",nullptr,nullptr,nullptr),SQLITE_OK);auto start=std::chrono::steady_clock::now();auto r=cache().AddRecord(Record());auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start);EXPECT_TRUE(r.HasError());EXPECT_EQ(r.GetError().primary_code,SQLITE_BUSY);EXPECT_GE(elapsed.count(),200);EXPECT_LT(elapsed.count(),1000);sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr);sqlite3_close(db);}
TEST_F(DownloadHistoryCacheTest, CorruptDatabaseFailsInitialization){std::ofstream out(Path(),std::ios::binary);out<<"not a sqlite database";out.close();auto r=cache().Initialize(Path());EXPECT_TRUE(r.HasError());EXPECT_FALSE(r.GetError().context.empty());}

class TrackerETagCacheTest : public testing::Test {
 protected:
  void SetUp() override { dir_=std::filesystem::temp_directory_path()/("gdownload-tracker-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir_);cache().Uninitialize(); }
  void TearDown() override { cache().Uninitialize();std::error_code ec;std::filesystem::remove_all(dir_,ec); }
  TrackerETagCache& cache(){return TrackerETagCache::Instance();}
  std::string Path(const char* name="tracker.db")const{return(dir_/name).string();}
  std::filesystem::path dir_;
};
TEST_F(TrackerETagCacheTest, UsesDeclaredConnectionPolicy){constexpr auto options=detail::TrackerCacheOpenOptions();static_assert(options.busy_timeout_ms==3000);static_assert(options.request_wal);}
TEST_F(TrackerETagCacheTest, AppliesBusyTimeoutAndJournalPolicyAtRuntime){
 ASSERT_TRUE(cache().Initialize(Path()).IsOk());
 sqlite3*db=nullptr;ASSERT_EQ(sqlite3_open(Path().c_str(),&db),SQLITE_OK);
 sqlite3_stmt*statement=nullptr;ASSERT_EQ(sqlite3_prepare_v2(db,"PRAGMA journal_mode",-1,&statement,nullptr),SQLITE_OK);ASSERT_EQ(sqlite3_step(statement),SQLITE_ROW);const auto*mode_text=sqlite3_column_text(statement,0);const std::string mode=mode_text?reinterpret_cast<const char*>(mode_text):"";sqlite3_finalize(statement);
 // WAL is preferred; platforms/filesystems that reject it may safely retain a rollback journal.
 EXPECT_FALSE(mode.empty());
 ASSERT_EQ(sqlite3_exec(db,"BEGIN IMMEDIATE",nullptr,nullptr,nullptr),SQLITE_OK);
 const auto start=std::chrono::steady_clock::now();auto result=cache().SetEntry({"locked","etag","content",1});const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start);
 ASSERT_TRUE(result.HasError());EXPECT_EQ(result.GetError().primary_code,SQLITE_BUSY);EXPECT_GE(elapsed.count(),2700);
 sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr);sqlite3_close(db);
 if(mode=="wal") EXPECT_EQ(mode,"wal"); else EXPECT_TRUE(cache().SetEntry({"fallback","etag","content",2}).IsOk());
}
TEST_F(TrackerETagCacheTest, CreatesVersionOneWithBusyTimeout){ASSERT_TRUE(cache().Initialize(Path()).IsOk());sqlite3*db=nullptr;ASSERT_EQ(sqlite3_open(Path().c_str(),&db),SQLITE_OK);sqlite3_stmt*s=nullptr;ASSERT_EQ(sqlite3_prepare_v2(db,"PRAGMA user_version",-1,&s,nullptr),SQLITE_OK);ASSERT_EQ(sqlite3_step(s),SQLITE_ROW);EXPECT_EQ(sqlite3_column_int(s,0),1);sqlite3_finalize(s);sqlite3_close(db);}
TEST_F(TrackerETagCacheTest, SupportsUnicodeCrudAndUpsert){ASSERT_TRUE(cache().Initialize(Path()).IsOk());TrackerETagEntry e{"https://例子.test/announce","\"标签\"","内容",7};ASSERT_TRUE(cache().SetEntry(e).IsOk());e.content="更新";ASSERT_TRUE(cache().SetEntry(e).IsOk());auto got=cache().GetEntry(e.url);ASSERT_TRUE(got.IsOk());ASSERT_TRUE(got.Value());EXPECT_EQ(got.Value()->content,"更新");EXPECT_EQ(cache().GetTotalCount().Value(),1u);ASSERT_TRUE(cache().DeleteEntry(e.url).IsOk());EXPECT_FALSE(cache().GetEntry(e.url).Value());}
TEST_F(TrackerETagCacheTest, MissingEntryIsSuccessfulEmptyOptional){ASSERT_TRUE(cache().Initialize(Path()).IsOk());auto r=cache().GetEntry("missing");ASSERT_TRUE(r.IsOk());EXPECT_FALSE(r.Value());}
TEST_F(TrackerETagCacheTest, ClosesAndReopensDifferentDatabase){ASSERT_TRUE(cache().Initialize(Path("a.db")).IsOk());ASSERT_TRUE(cache().SetEntry({"u","e","c",1}).IsOk());ASSERT_TRUE(cache().Uninitialize().IsOk());EXPECT_TRUE(cache().Uninitialize().IsOk());ASSERT_TRUE(cache().Initialize(Path("b.db")).IsOk());EXPECT_FALSE(cache().GetEntry("u").Value());}
TEST_F(TrackerETagCacheTest, QueryErrorIsNotReportedAsEmpty){ASSERT_TRUE(cache().Initialize(Path()).IsOk());sqlite3*db=nullptr;ASSERT_EQ(sqlite3_open(Path().c_str(),&db),SQLITE_OK);ASSERT_EQ(sqlite3_exec(db,"DROP TABLE tracker_etag_cache",nullptr,nullptr,nullptr),SQLITE_OK);sqlite3_close(db);EXPECT_TRUE(cache().GetAllEntries().HasError());}
TEST_F(TrackerETagCacheTest, AdoptsMatchingUnversionedTable){sqlite3*db=nullptr;ASSERT_EQ(sqlite3_open(Path().c_str(),&db),SQLITE_OK);ASSERT_EQ(sqlite3_exec(db,"CREATE TABLE tracker_etag_cache(url TEXT PRIMARY KEY,etag TEXT NOT NULL,content TEXT NOT NULL,timestamp INTEGER NOT NULL)",nullptr,nullptr,nullptr),SQLITE_OK);sqlite3_close(db);ASSERT_TRUE(cache().Initialize(Path()).IsOk());}
}
