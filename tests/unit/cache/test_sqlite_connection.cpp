#include <gtest/gtest.h>

#include <sqlite3.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>

#include "cache/sqlite_connection.h"

namespace {

using gdl::cache::CacheOperation;
using gdl::cache::CacheResult;
using gdl::cache::SqliteConnection;
using gdl::cache::SqliteOpenOptions;

class TemporaryDirectory {
   public:
	TemporaryDirectory() {
		path_ = std::filesystem::temp_directory_path() /
			("gdownload-sqlite-" + std::to_string(
				std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::create_directories(path_);
	}
	~TemporaryDirectory() {
		std::error_code error;
		std::filesystem::remove_all(path_, error);
	}
	[[nodiscard]] const std::filesystem::path& Path() const { return path_; }

   private:
	std::filesystem::path path_;
};

int ReadIntPragma(sqlite3* db, const char* sql) {
	sqlite3_stmt* statement = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) return -1;
	const int value = sqlite3_step(statement) == SQLITE_ROW ? sqlite3_column_int(statement, 0) : -1;
	sqlite3_finalize(statement);
	return value;
}

std::string ReadTextPragma(sqlite3* db, const char* sql) {
	sqlite3_stmt* statement = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) return {};
	std::string value;
	if (sqlite3_step(statement) == SQLITE_ROW) {
		const auto* text = sqlite3_column_text(statement, 0);
		if (text) value = reinterpret_cast<const char*>(text);
	}
	sqlite3_finalize(statement);
	return value;
}

TEST(SqliteConnectionTest, AppliesConnectionLocalConfiguration) {
	TemporaryDirectory directory;
	SqliteConnection connection;

	auto opened = connection.Open((directory.Path() / "cache.db").string(),
		{.busy_timeout_ms = 4321, .request_wal = true});

	ASSERT_TRUE(opened.IsOk()) << opened.GetError().Describe();
	auto values = connection.Use<std::pair<int, int>>(CacheOperation::kInspect, "read pragmas",
		[](sqlite3* db) {
			return CacheResult<std::pair<int, int>>::Success(
				std::pair{ReadIntPragma(db, "PRAGMA busy_timeout"),
					ReadIntPragma(db, "PRAGMA synchronous")});
		});
	ASSERT_TRUE(values.IsOk());
	EXPECT_EQ(values.Value().first, 4321);
	EXPECT_EQ(values.Value().second, 1);
}

TEST(SqliteConnectionTest, UsesWalWhenSupported) {
	TemporaryDirectory directory;
	SqliteConnection connection;
	ASSERT_TRUE(connection.Open((directory.Path() / "wal.db").string()).IsOk());

	auto journal_mode = connection.Use<std::string>(CacheOperation::kInspect, "journal mode",
		[](sqlite3* db) { return CacheResult<std::string>::Success(ReadTextPragma(db, "PRAGMA journal_mode")); });

	ASSERT_TRUE(journal_mode.IsOk());
	EXPECT_EQ(journal_mode.Value(), "wal");
}

TEST(SqliteConnectionTest, ClosesAndReopensAnotherPath) {
	TemporaryDirectory directory;
	SqliteConnection connection;
	const std::string first = (directory.Path() / "first.db").string();
	const std::string second = (directory.Path() / "second.db").string();
	ASSERT_TRUE(connection.Open(first).IsOk());
	EXPECT_EQ(connection.Path(), first);
	ASSERT_TRUE(connection.Close().IsOk());
	EXPECT_FALSE(connection.IsOpen());
	ASSERT_TRUE(connection.Open(second).IsOk());
	EXPECT_EQ(connection.Path(), second);
}

TEST(SqliteConnectionTest, ReportsDirectoryCreationFailure) {
	TemporaryDirectory directory;
	const auto parent_file = directory.Path() / "not-a-directory";
	ASSERT_TRUE(std::ofstream(parent_file).good());
	const std::string path = (parent_file / "cache.db").string();
	SqliteConnection connection;

	auto result = connection.Open(path);

	ASSERT_TRUE(result.HasError());
	EXPECT_EQ(result.GetError().operation, CacheOperation::kCreateDirectory);
	EXPECT_EQ(result.GetError().path, path);
	EXPECT_FALSE(result.GetError().context.empty());
}

TEST(SqliteConnectionTest, OpenFailureIncludesSqliteErrorFields) {
	TemporaryDirectory directory;
	const std::string path = directory.Path().string();
	SqliteConnection connection;

	auto result = connection.Open(path);

	ASSERT_TRUE(result.HasError());
	EXPECT_EQ(result.GetError().operation, CacheOperation::kOpen);
	EXPECT_EQ(result.GetError().path, path);
	EXPECT_NE(result.GetError().primary_code, 0);
	EXPECT_NE(result.GetError().extended_code, 0);
	EXPECT_FALSE(result.GetError().context.empty());
	EXPECT_FALSE(connection.IsOpen());
}

TEST(SqliteConnectionTest, CloseWaitsForActiveUse) {
	TemporaryDirectory directory;
	SqliteConnection connection;
	ASSERT_TRUE(connection.Open((directory.Path() / "locked.db").string()).IsOk());
	std::promise<void> entered;
	std::promise<void> release;
	auto release_future = release.get_future().share();
	auto use_future = std::async(std::launch::async, [&] {
		return connection.Use<void>(CacheOperation::kInspect, "blocking callback", [&](sqlite3*) {
			entered.set_value();
			release_future.wait();
			return CacheResult<void>::Success();
		});
	});
	entered.get_future().wait();
	auto close_future = std::async(std::launch::async, [&] { return connection.Close(); });

	EXPECT_EQ(close_future.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);
	release.set_value();
	EXPECT_TRUE(use_future.get().IsOk());
	EXPECT_TRUE(close_future.get().IsOk());
}

TEST(SqliteConnectionTest, CloseIsIdempotent) {
	SqliteConnection connection;
	EXPECT_TRUE(connection.Close().IsOk());
	EXPECT_TRUE(connection.Close().IsOk());
}

TEST(SqliteConnectionTest, RejectsSameConnectionReentryFromUseCallback) {
	TemporaryDirectory directory;
	SqliteConnection connection;
	ASSERT_TRUE(connection.Open((directory.Path() / "reentry.db").string()).IsOk());

	auto result = connection.Use<void>(CacheOperation::kInspect, "reentry", [&](sqlite3*) {
		EXPECT_THROW(static_cast<void>(connection.Path()), std::logic_error);
		EXPECT_THROW(static_cast<void>(connection.IsOpen()), std::logic_error);
		EXPECT_THROW(connection.Close(), std::logic_error);
		EXPECT_THROW(connection.Use<void>(CacheOperation::kInspect, "nested",
			[](sqlite3*) { return CacheResult<void>::Success(); }), std::logic_error);
		return CacheResult<void>::Success();
	});

	EXPECT_TRUE(result.IsOk());
}

TEST(SqliteConnectionTest, PreservesCallbackFailure) {
	TemporaryDirectory directory;
	SqliteConnection connection;
	const std::string path = (directory.Path() / "failure.db").string();
	ASSERT_TRUE(connection.Open(path).IsOk());

	auto result = connection.Use<int>(CacheOperation::kStep, "callback failure", [&](sqlite3*) {
		return CacheResult<int>::Failure({.operation = CacheOperation::kStep,
			.primary_code = SQLITE_CONSTRAINT,
			.extended_code = SQLITE_CONSTRAINT_UNIQUE,
			.path = path,
			.context = "expected failure"});
	});

	ASSERT_TRUE(result.HasError());
	EXPECT_EQ(result.GetError().extended_code, SQLITE_CONSTRAINT_UNIQUE);
}

TEST(SqliteConnectionTest, CallbackExceptionPropagatesAndReleasesReentryGuard) {
	TemporaryDirectory directory;
	SqliteConnection connection;
	ASSERT_TRUE(connection.Open((directory.Path() / "exception.db").string()).IsOk());

	EXPECT_THROW(connection.Use<void>(CacheOperation::kInspect, "throws",
		[](sqlite3*) -> CacheResult<void> { throw std::runtime_error("callback failed"); }),
		std::runtime_error);

	EXPECT_TRUE(connection.IsOpen());
	EXPECT_TRUE(connection.Close().IsOk());
}

TEST(SqliteConnectionTest, OpensUnicodeAndSpaceUtf8Path) {
	TemporaryDirectory directory;
	const std::string unicode_name = "\xE4\xB8\x8B\xE8\xBD\xBD \xE6\x95\xB0\xE6\x8D\xAE";
	const auto filesystem_path = directory.Path() / std::filesystem::u8path(unicode_name) / "cache file.db";
#if defined(__cpp_lib_char8_t)
	const auto utf8 = filesystem_path.u8string();
	const std::string database_path(reinterpret_cast<const char*>(utf8.data()), utf8.size());
#else
	const std::string database_path = filesystem_path.u8string();
#endif
	SqliteConnection connection;

	auto opened = connection.Open(database_path);

	ASSERT_TRUE(opened.IsOk()) << opened.GetError().Describe();
	auto pragma = connection.Use<int>(CacheOperation::kInspect, "unicode path pragma",
		[](sqlite3* db) { return CacheResult<int>::Success(ReadIntPragma(db, "PRAGMA user_version")); });
	ASSERT_TRUE(pragma.IsOk());
	EXPECT_EQ(pragma.Value(), 0);
	EXPECT_TRUE(std::filesystem::exists(filesystem_path));
}

}  // namespace
