#pragma once
#include <string>
#include <vector>
#include "cache_result.h"
#include "export.h"

namespace gdl::cache {
class SqliteConnection;
struct ColumnDefinition { std::string name; std::string type; bool not_null{false}; std::string default_value; int primary_key_position{0}; };
struct MigrationStep { int from_version{0}; int to_version{0}; std::vector<std::string> sql; };
struct SchemaDefinition { std::string table_name; int current_version{0}; std::string create_table_sql; std::vector<ColumnDefinition> current_columns; std::vector<MigrationStep> migrations; };
GDLCore_API CacheResult<void> EnsureSchema(SqliteConnection& connection, const SchemaDefinition& definition);
}
