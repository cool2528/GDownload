#include <gtest/gtest.h>
#include <sqlite3.h>
#include <chrono>
#include <filesystem>

#include "cache/schema_migrator.h"
#include "cache/sqlite_connection.h"

namespace {
using namespace gdl::cache;

struct DbFixture {
	std::filesystem::path dir = std::filesystem::temp_directory_path() /
		("gdownload-schema-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	SqliteConnection connection;
	DbFixture() { std::filesystem::create_directories(dir); EXPECT_TRUE(connection.Open((dir / "db.sqlite").string()).IsOk()); }
	~DbFixture() { connection.Close(); std::error_code ec; std::filesystem::remove_all(dir, ec); }
	void Exec(const char* sql) { ASSERT_TRUE(connection.Use<void>(CacheOperation::kStep, sql, [=](sqlite3* db, const std::string& database_path) {
		return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK ? CacheResult<void>::Success() :
			CacheResult<void>::Failure({.operation=CacheOperation::kStep,.primary_code=sqlite3_errcode(db),.extended_code=sqlite3_extended_errcode(db),.context=sqlite3_errmsg(db)});
	}).IsOk()); }
	int Int(const char* sql) { auto r=connection.Use<int>(CacheOperation::kInspect, sql,[=](sqlite3* db, const std::string& database_path){sqlite3_stmt* s=nullptr; sqlite3_prepare_v2(db,sql,-1,&s,nullptr); int v=sqlite3_step(s)==SQLITE_ROW?sqlite3_column_int(s,0):-1; sqlite3_finalize(s); return CacheResult<int>::Success(v);}); return r.Value(); }
};

SchemaDefinition CurrentSchema() {
	return {.table_name="items", .current_version=3,
		.create_table_sql="CREATE TABLE items(id INTEGER PRIMARY KEY, name TEXT NOT NULL)",
		.current_columns={{"id","INTEGER",false,"",true},{"name","TEXT",true,"",false}},
		.migrations={{1,2,{"ALTER TABLE items RENAME TO items_old", "CREATE TABLE items(id INTEGER PRIMARY KEY, name TEXT NOT NULL)", "INSERT INTO items(id,name) SELECT id,'' FROM items_old", "DROP TABLE items_old"}},
			{2,3,{}}}};
}

TEST(SchemaMigratorTest, CreatesMissingSchemaAtCurrentVersion) { DbFixture f; EXPECT_TRUE(EnsureSchema(f.connection,CurrentSchema()).IsOk()); EXPECT_EQ(f.Int("PRAGMA user_version"),3); EXPECT_EQ(f.Int("SELECT count(*) FROM sqlite_master WHERE name='items'"),1); }
TEST(SchemaMigratorTest, AdoptsMatchingUnversionedSchema) { DbFixture f; f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY, name TEXT NOT NULL)"); EXPECT_TRUE(EnsureSchema(f.connection,CurrentSchema()).IsOk()); EXPECT_EQ(f.Int("PRAGMA user_version"),3); }
TEST(SchemaMigratorTest, AppliesOrderedMigrationChain) { DbFixture f; f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY)"); f.Exec("PRAGMA user_version=1"); f.Exec("INSERT INTO items VALUES(7)"); auto result=EnsureSchema(f.connection,CurrentSchema()); ASSERT_TRUE(result.IsOk()) << result.GetError().Describe(); EXPECT_EQ(f.Int("PRAGMA user_version"),3); EXPECT_EQ(f.Int("SELECT count(*) FROM items WHERE id=7"),1); }
TEST(SchemaMigratorTest, RejectsUnknownUnversionedSchemaWithoutDeletingIt) { DbFixture f; f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY, alien BLOB)"); auto r=EnsureSchema(f.connection,CurrentSchema()); EXPECT_TRUE(r.HasError()); EXPECT_EQ(r.GetError().operation,CacheOperation::kInspect); EXPECT_EQ(f.Int("PRAGMA user_version"),0); EXPECT_EQ(f.Int("SELECT count(*) FROM pragma_table_info('items') WHERE name='alien'"),1); }
TEST(SchemaMigratorTest, RejectsFutureVersion) { DbFixture f; f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY, name TEXT NOT NULL)"); f.Exec("PRAGMA user_version=9"); auto r=EnsureSchema(f.connection,CurrentSchema()); EXPECT_TRUE(r.HasError()); EXPECT_EQ(r.GetError().operation,CacheOperation::kInspect); EXPECT_EQ(f.Int("PRAGMA user_version"),9); }
TEST(SchemaMigratorTest, FailedMigrationRollsBackSchemaVersionAndData) { DbFixture f; auto d=CurrentSchema(); d.migrations[0].sql.push_back("INSERT INTO missing_table VALUES(1)"); f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY)"); f.Exec("PRAGMA user_version=1"); f.Exec("INSERT INTO items VALUES(8)"); auto r=EnsureSchema(f.connection,d); EXPECT_TRUE(r.HasError()); EXPECT_EQ(r.GetError().operation,CacheOperation::kMigrate); EXPECT_EQ(f.Int("PRAGMA user_version"),1); EXPECT_EQ(f.Int("SELECT count(*) FROM items WHERE id=8"),1); EXPECT_EQ(f.Int("SELECT count(*) FROM pragma_table_info('items') WHERE name='name'"),0); }
TEST(SchemaMigratorTest, RejectsCurrentVersionWithMismatchedSchema) { DbFixture f; f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY, alien TEXT)"); f.Exec("PRAGMA user_version=3"); f.Exec("INSERT INTO items VALUES(4,'keep')"); auto r=EnsureSchema(f.connection,CurrentSchema()); EXPECT_TRUE(r.HasError()); EXPECT_EQ(r.GetError().operation,CacheOperation::kInspect); EXPECT_EQ(f.Int("PRAGMA user_version"),3); EXPECT_EQ(f.Int("SELECT count(*) FROM items WHERE id=4"),1); }
TEST(SchemaMigratorTest, RollsBackMigrationWhenFinalColumnsDoNotMatch) { DbFixture f; auto d=CurrentSchema(); d.migrations[0].sql={"ALTER TABLE items ADD COLUMN wrong TEXT"}; d.migrations[1].sql.clear(); f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY)"); f.Exec("PRAGMA user_version=1"); f.Exec("INSERT INTO items VALUES(5)"); auto r=EnsureSchema(f.connection,d); EXPECT_TRUE(r.HasError()); EXPECT_EQ(r.GetError().operation,CacheOperation::kMigrate); EXPECT_EQ(f.Int("PRAGMA user_version"),1); EXPECT_EQ(f.Int("SELECT count(*) FROM items WHERE id=5"),1); EXPECT_EQ(f.Int("SELECT count(*) FROM pragma_table_info('items') WHERE name='wrong'"),0); }
TEST(SchemaMigratorTest, RollsBackCreateWhenDeclaredColumnsDoNotMatch) { DbFixture f; auto d=CurrentSchema(); d.create_table_sql="CREATE TABLE items(id INTEGER PRIMARY KEY, wrong BLOB)"; auto r=EnsureSchema(f.connection,d); EXPECT_TRUE(r.HasError()); EXPECT_EQ(f.Int("PRAGMA user_version"),0); EXPECT_EQ(f.Int("SELECT count(*) FROM sqlite_master WHERE name='items'"),0); }
TEST(SchemaMigratorTest, RejectsCompositePrimaryKeyOrderMismatch) { DbFixture f; SchemaDefinition d{.table_name="pairs",.current_version=1,.create_table_sql="CREATE TABLE pairs(a INTEGER,b INTEGER,PRIMARY KEY(a,b))",.current_columns={{"a","INTEGER",false,"",2},{"b","INTEGER",false,"",1}}}; f.Exec("CREATE TABLE pairs(a INTEGER,b INTEGER,PRIMARY KEY(a,b))"); auto r=EnsureSchema(f.connection,d); EXPECT_TRUE(r.HasError()); EXPECT_EQ(r.GetError().operation,CacheOperation::kInspect); }
TEST(SchemaMigratorTest, RejectsInvalidMigrationDefinitionForNewDatabase) { DbFixture f; auto d=CurrentSchema(); d.migrations.push_back({1,2,{"SELECT 1"}}); auto r=EnsureSchema(f.connection,d); EXPECT_TRUE(r.HasError()); EXPECT_EQ(r.GetError().operation,CacheOperation::kMigrate); EXPECT_EQ(f.Int("SELECT count(*) FROM sqlite_master WHERE name='items'"),0); }
TEST(SchemaMigratorTest, RejectsInvalidMigrationDefinitionForCurrentDatabase) { DbFixture f; auto d=CurrentSchema(); d.migrations[0].to_version=3; f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY,name TEXT NOT NULL)"); f.Exec("PRAGMA user_version=3"); auto r=EnsureSchema(f.connection,d); EXPECT_TRUE(r.HasError()); EXPECT_EQ(f.Int("PRAGMA user_version"),3); }
TEST(SchemaMigratorTest, RejectsTransactionControlWithoutPartialCommit) { DbFixture f; auto d=CurrentSchema(); d.migrations[0].sql={"ALTER TABLE items ADD COLUMN name TEXT NOT NULL DEFAULT ''","COMMIT"}; f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY)"); f.Exec("PRAGMA user_version=1"); f.Exec("INSERT INTO items VALUES(9)"); auto r=EnsureSchema(f.connection,d); EXPECT_TRUE(r.HasError()); EXPECT_EQ(f.Int("PRAGMA user_version"),1); EXPECT_EQ(f.Int("SELECT count(*) FROM pragma_table_info('items') WHERE name='name'"),0); EXPECT_EQ(f.Int("SELECT count(*) FROM items WHERE id=9"),1); }
TEST(SchemaMigratorTest, RejectsNegativeUserVersionWithoutChangingData) { DbFixture f; f.Exec("CREATE TABLE items(id INTEGER PRIMARY KEY,name TEXT NOT NULL)"); f.Exec("INSERT INTO items VALUES(11,'keep')"); f.Exec("PRAGMA user_version=-1"); auto r=EnsureSchema(f.connection,CurrentSchema()); ASSERT_TRUE(r.HasError()); EXPECT_EQ(r.GetError().operation,CacheOperation::kInspect); EXPECT_EQ(r.GetError().primary_code,SQLITE_SCHEMA); EXPECT_EQ(f.Int("PRAGMA user_version"),-1); EXPECT_EQ(f.Int("SELECT count(*) FROM items WHERE id=11"),1); }
}

