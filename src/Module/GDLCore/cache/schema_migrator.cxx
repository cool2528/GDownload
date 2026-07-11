#include "schema_migrator.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <set>
#include <sstream>

#include "sqlite_connection.h"

namespace gdl::cache {
namespace {

using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

CacheError MakeError(sqlite3* db, CacheOperation operation, const std::string& path,
	const std::string& context, int code) {
	const int extended = db ? sqlite3_extended_errcode(db) : code;
	return {.operation = operation, .primary_code = code & 0xff,
		.extended_code = extended, .path = path,
		.context = context + (db ? ": " + std::string(sqlite3_errmsg(db)) : "")};
}

bool IsIdentifier(const std::string& value) {
	return !value.empty() && (std::isalpha(static_cast<unsigned char>(value[0])) || value[0] == '_') &&
		std::all_of(value.begin() + 1, value.end(), [](char c) {
			return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
		});
}

std::string FirstToken(const std::string& sql) {
	size_t pos = 0;
	while (pos < sql.size()) {
		while (pos < sql.size() && std::isspace(static_cast<unsigned char>(sql[pos]))) ++pos;
		if (sql.compare(pos, 2, "--") == 0) { pos = sql.find('\n', pos + 2); if (pos == std::string::npos) return {}; continue; }
		if (sql.compare(pos, 2, "/*") == 0) { pos = sql.find("*/", pos + 2); if (pos == std::string::npos) return {}; pos += 2; continue; }
		break;
	}
	const size_t begin = pos;
	while (pos < sql.size() && std::isalpha(static_cast<unsigned char>(sql[pos]))) ++pos;
	std::string token = sql.substr(begin, pos - begin);
	std::transform(token.begin(), token.end(), token.begin(), [](char c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); });
	return token;
}

bool IsTransactionControl(const std::string& sql) {
	static const std::set<std::string> denied{"BEGIN", "COMMIT", "END", "ROLLBACK", "SAVEPOINT", "RELEASE"};
	return denied.contains(FirstToken(sql));
}

CacheResult<void> ExecuteOne(sqlite3* db, const std::string& path, const std::string& sql,
	CacheOperation operation, bool require_transaction = true) {
	if (IsTransactionControl(sql)) return CacheResult<void>::Failure({.operation = operation,
		.primary_code = SQLITE_MISUSE, .extended_code = SQLITE_MISUSE, .path = path,
		.context = "transaction control is not allowed in migration SQL"});
	sqlite3_stmt* raw = nullptr;
	const char* tail = nullptr;
	const int prepare_code = sqlite3_prepare_v2(db, sql.c_str(), -1, &raw, &tail);
	Statement statement(raw, sqlite3_finalize);
	if (prepare_code != SQLITE_OK) return CacheResult<void>::Failure(MakeError(db, operation, path, "prepare migration statement", prepare_code));
	while (tail && *tail && std::isspace(static_cast<unsigned char>(*tail))) ++tail;
	if (tail && *tail) return CacheResult<void>::Failure({.operation = operation,
		.primary_code = SQLITE_MISUSE, .extended_code = SQLITE_MISUSE, .path = path,
		.context = "migration entry must contain exactly one SQL statement"});
	if (require_transaction && sqlite3_get_autocommit(db) != 0) return CacheResult<void>::Failure({.operation = operation,
		.primary_code = SQLITE_MISUSE, .extended_code = SQLITE_MISUSE, .path = path, .context = "migration transaction is not active"});
	const int step_code = sqlite3_step(statement.get());
	if (step_code != SQLITE_DONE) return CacheResult<void>::Failure(MakeError(db, operation, path, "step migration statement", step_code));
	if (require_transaction && sqlite3_get_autocommit(db) != 0) return CacheResult<void>::Failure({.operation = operation,
		.primary_code = SQLITE_MISUSE, .extended_code = SQLITE_MISUSE, .path = path, .context = "migration statement ended transaction"});
	return CacheResult<void>::Success();
}

CacheResult<void> ValidateDefinition(const SchemaDefinition& definition) {
	if (!IsIdentifier(definition.table_name) || definition.current_version <= 0)
		return CacheResult<void>::Failure({.operation = CacheOperation::kInspect,
			.primary_code = SQLITE_MISUSE, .extended_code = SQLITE_MISUSE,
			.context = "invalid schema definition"});
	std::set<int> from_versions;
	for (const auto& migration : definition.migrations) {
		if (migration.from_version < 1 || migration.to_version != migration.from_version + 1 ||
			migration.to_version > definition.current_version || !from_versions.insert(migration.from_version).second)
			return CacheResult<void>::Failure({.operation = CacheOperation::kMigrate,
				.primary_code = SQLITE_SCHEMA, .extended_code = SQLITE_SCHEMA,
				.context = "invalid migration definition"});
	}
	for (int version = 1; version < definition.current_version; ++version)
		if (!from_versions.contains(version)) return CacheResult<void>::Failure({.operation = CacheOperation::kMigrate,
			.primary_code = SQLITE_SCHEMA, .extended_code = SQLITE_SCHEMA,
			.context = "migration definition is incomplete"});
	return CacheResult<void>::Success();
}

std::vector<ColumnDefinition> ReadColumns(sqlite3* db, const std::string& path, const std::string& table,
	CacheError* error) {
	const std::string sql = "PRAGMA table_info(\"" + table + "\")";
	sqlite3_stmt* raw = nullptr;
	const int prepare_code = sqlite3_prepare_v2(db, sql.c_str(), -1, &raw, nullptr);
	Statement statement(raw, sqlite3_finalize);
	if (prepare_code != SQLITE_OK) { *error = MakeError(db, CacheOperation::kInspect, path, "prepare table_info", prepare_code); return {}; }
	std::vector<ColumnDefinition> columns;
	for (;;) {
		const int code = sqlite3_step(statement.get());
		if (code == SQLITE_DONE) break;
		if (code != SQLITE_ROW) { *error = MakeError(db, CacheOperation::kInspect, path, "step table_info", code); return {}; }
		auto text = [&](int index) { const auto* value = sqlite3_column_text(statement.get(), index); return value ? std::string(reinterpret_cast<const char*>(value)) : std::string(); };
		columns.push_back({text(1), text(2), sqlite3_column_int(statement.get(), 3) != 0,
			text(4), sqlite3_column_int(statement.get(), 5)});
	}
	return columns;
}

bool SameColumns(const std::vector<ColumnDefinition>& left, const std::vector<ColumnDefinition>& right) {
	return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), [](const auto& a, const auto& b) {
		return a.name == b.name && a.type == b.type && a.not_null == b.not_null &&
			a.default_value == b.default_value && a.primary_key_position == b.primary_key_position;
	});
}
}  // namespace

CacheResult<void> EnsureSchema(SqliteConnection& connection, const SchemaDefinition& definition) {
	auto valid = ValidateDefinition(definition);
	if (valid.HasError()) return valid;
	return connection.Use<void>(CacheOperation::kMigrate, "ensure schema", [&](sqlite3* db, const std::string& database_path) {
		const std::string& path = database_path;
		auto begin = ExecuteOne(db, path, "BEGIN IMMEDIATE", CacheOperation::kMigrate, false);
		// BEGIN is intentionally issued by the migrator, not supplied by a migration step.
		if (begin.HasError()) {
			const int code = sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
			if (code != SQLITE_OK) return CacheResult<void>::Failure(MakeError(db, CacheOperation::kMigrate, path, "begin transaction", code));
		}
		bool active = true;
		auto rollback = [&](CacheError original) {
			const int rollback_code = sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
			active = false;
			if (rollback_code != SQLITE_OK) original.context += "; rollback failed: " + std::string(sqlite3_errmsg(db));
			return CacheResult<void>::Failure(std::move(original));
		};
		try {
			int version = -1;
			sqlite3_stmt* raw = nullptr;
			int code = sqlite3_prepare_v2(db, "PRAGMA user_version", -1, &raw, nullptr);
			Statement user_version(raw, sqlite3_finalize);
			if (code != SQLITE_OK) return rollback(MakeError(db, CacheOperation::kInspect, path, "prepare user_version", code));
			code = sqlite3_step(user_version.get());
			if (code != SQLITE_ROW) return rollback(MakeError(db, CacheOperation::kInspect, path, "step user_version", code));
			version = sqlite3_column_int(user_version.get(), 0);
			user_version.reset();
			const int initial_version = version;
			if (version < 0) return rollback({.operation = CacheOperation::kInspect,
				.primary_code = SQLITE_SCHEMA, .extended_code = SQLITE_SCHEMA, .path = path,
				.context = "negative schema version"});

			raw = nullptr;
			code = sqlite3_prepare_v2(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?", -1, &raw, nullptr);
			Statement exists_statement(raw, sqlite3_finalize);
			if (code != SQLITE_OK) return rollback(MakeError(db, CacheOperation::kInspect, path, "prepare table existence", code));
			code = sqlite3_bind_text(exists_statement.get(), 1, definition.table_name.c_str(), -1, SQLITE_TRANSIENT);
			if (code != SQLITE_OK) return rollback(MakeError(db, CacheOperation::kBind, path, "bind table name", code));
			code = sqlite3_step(exists_statement.get());
			if (code != SQLITE_ROW) return rollback(MakeError(db, CacheOperation::kInspect, path, "step table existence", code));
			const bool exists = sqlite3_column_int(exists_statement.get(), 0) > 0;
			exists_statement.reset();

			if (!exists) {
				if (version != 0) return rollback({.operation = CacheOperation::kInspect, .primary_code = SQLITE_SCHEMA, .extended_code = SQLITE_SCHEMA, .path = path, .context = "versioned database is missing table"});
				auto created = ExecuteOne(db, path, definition.create_table_sql, CacheOperation::kMigrate);
				if (created.HasError()) return rollback(created.GetError());
				version = definition.current_version;
			} else if (version == 0) {
				CacheError inspect_error;
				if (!SameColumns(ReadColumns(db, path, definition.table_name, &inspect_error), definition.current_columns))
					return rollback(inspect_error.primary_code ? inspect_error : CacheError{.operation = CacheOperation::kInspect, .primary_code = SQLITE_SCHEMA, .extended_code = SQLITE_SCHEMA, .path = path, .context = "unrecognized unversioned schema"});
				version = definition.current_version;
			} else if (version > definition.current_version) {
				return rollback({.operation = CacheOperation::kInspect, .primary_code = SQLITE_SCHEMA, .extended_code = SQLITE_SCHEMA, .path = path, .context = "future schema version"});
			}

			while (version < definition.current_version) {
				const auto migration_it = std::find_if(definition.migrations.begin(), definition.migrations.end(), [&](const auto& item) { return item.from_version == version; });
				if (migration_it == definition.migrations.end()) return rollback({.operation = CacheOperation::kMigrate,
					.primary_code = SQLITE_SCHEMA, .extended_code = SQLITE_SCHEMA, .path = path,
					.context = "migration step is unavailable"});
				const auto& migration = *migration_it;
				for (const auto& sql : migration.sql) {
					auto executed = ExecuteOne(db, path, sql, CacheOperation::kMigrate);
					if (executed.HasError()) return rollback(executed.GetError());
				}
				version = migration.to_version;
			}

			CacheError inspect_error;
			if (!SameColumns(ReadColumns(db, path, definition.table_name, &inspect_error), definition.current_columns))
				return rollback(inspect_error.primary_code ? inspect_error : CacheError{.operation = initial_version == definition.current_version ? CacheOperation::kInspect : CacheOperation::kMigrate, .primary_code = SQLITE_SCHEMA, .extended_code = SQLITE_SCHEMA, .path = path, .context = "final schema does not match definition"});

			std::ostringstream pragma;
			pragma << "PRAGMA user_version=" << version;
			auto set_version = ExecuteOne(db, path, pragma.str(), CacheOperation::kMigrate);
			if (set_version.HasError()) return rollback(set_version.GetError());
			code = sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
			if (code != SQLITE_OK) return rollback(MakeError(db, CacheOperation::kCommit, path, "commit migration", code));
			active = false;
			return CacheResult<void>::Success();
		} catch (...) {
			if (active) sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
			throw;
		}
	});
}
}  // namespace gdl::cache

