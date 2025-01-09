#include "Database.h"

#include <format>
#include <sqlite3.h>
#include <stdexcept>
#include <ranges>

void db::Database::openDatabase() {
	const int rc = sqlite3_open(db_path.c_str(), &db);
	if (rc != SQLITE_OK) {
		throw std::runtime_error(sqlite3_errmsg(db));
	}
}

void db::Database::closeDatabase() const {
	const int rc = sqlite3_close(db);
	if (rc != SQLITE_OK) {
		throw std::runtime_error(sqlite3_errmsg(db));
	}
}

void db::Database::createTable(const std::string &table, const std::vector<Column> &columns) const {
	// Validate input to ensure columns are provided
	if (columns.empty()) {
		throw std::invalid_argument("Cannot create a table without columns.");
	}

	// Construct the column definitions for the CREATE TABLE statement
	std::string columnDefinitions;
	for (const auto &[name, type, primaryKey, autoIncrement, notNull, unique, defaultVal, foreignKey]: columns) {
		if (!columnDefinitions.empty()) {
			columnDefinitions += ", ";
		}

		// Build a column definition based on Column attributes
		columnDefinitions += name + " " + type;

		if (primaryKey) {
			columnDefinitions += " PRIMARY KEY";
		}

		if (autoIncrement) {
			columnDefinitions += " AUTOINCREMENT";
		}

		if (notNull) {
			columnDefinitions += " NOT NULL";
		}

		if (unique) {
			columnDefinitions += " UNIQUE";
		}

		// Add any other constraints like CHECK, DEFAULT, or foreign key handling
		if (defaultVal.has_value()) {
			columnDefinitions += " DEFAULT " + defaultVal.value();
		}
		if (foreignKey.has_value()) {
			columnDefinitions += " REFERENCES " + foreignKey.value();
		}
	}

	// Construct the final CREATE TABLE SQL query
	const std::string query = std::format("CREATE TABLE IF NOT EXISTS {} ({})", table, columnDefinitions);

	// Prepare the SQLite statement
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
	}

	// Execute the query
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		const std::string error = "Failed to execute statement: " + std::string(sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		throw std::runtime_error(error);
	}

	// Finalize/close the prepared statement
	sqlite3_finalize(stmt);
}

void db::Database::addRecord(const Record &record) const {
	// Construct the SQL query
	std::string columns;
	std::string placeholders;

	for (const auto &key: record.data | std::views::keys) {
		if (!columns.empty()) {
			columns += ", ";
			placeholders += ", ";
		}
		columns += key;
		placeholders += "?";
	}
	const std::string query = std::format("INSERT INTO {} ({}) VALUES ({})", record.table, columns, placeholders);

	// Prepare the SQLite statement
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
	}

	// Bind the values from the RecordData
	int index = 1; // SQLite parameters are 1-indexed
	for (const auto &[key, value]: record.data) {
		if (std::holds_alternative<int>(value)) {
			sqlite3_bind_int(stmt, index, std::get<int>(value));
		} else if (std::holds_alternative<double>(value)) {
			sqlite3_bind_double(stmt, index, std::get<double>(value));
		} else if (std::holds_alternative<std::string>(value)) {
			sqlite3_bind_text(stmt, index, std::get<std::string>(value).c_str(), -1, SQLITE_STATIC);
		} else {
			sqlite3_finalize(stmt);
			throw std::runtime_error("Unsupported field type in record: " + key);
		}
		++index;
	}

	// Execute the query
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		const std::string error = "Failed to execute statement: " + std::string(sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		throw std::runtime_error(error);
	}

	sqlite3_finalize(stmt);
}

void db::Database::removeRecord(const std::string &table, const RecordData &data) const {
	// Construct the SQL query
	std::string whereClause;
	for (const auto &key: data | std::views::keys) {
		if (!whereClause.empty()) {
			whereClause += " AND "; // Add AND between conditions
		}
		whereClause += key + " = ?";
	}

	const std::string query = std::format("DELETE FROM {} WHERE {}", table, whereClause);

	// Prepare the SQLite statement
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
	}

	// Bind the parameters dynamically
	int index = 1;
	for (const auto &[key, value]: data) {
		if (std::holds_alternative<int>(value)) {
			sqlite3_bind_int(stmt, index, std::get<int>(value));
		} else if (std::holds_alternative<double>(value)) {
			sqlite3_bind_double(stmt, index, std::get<double>(value));
		} else if (std::holds_alternative<std::string>(value)) {
			sqlite3_bind_text(stmt, index, std::get<std::string>(value).c_str(), -1, SQLITE_STATIC);
		} else {
			sqlite3_finalize(stmt);
			throw std::runtime_error("Unsupported field type in data: " + key);
		}
		++index;
	}

	// Execute the query
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		const std::string error = "Failed to execute statement: " + std::string(sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		throw std::runtime_error(error);
	}

	sqlite3_finalize(stmt);
}

void db::Database::removeRecordByPseudoId(const std::string &table, const int pseudoId) const {
	const std::string query = R"(
        DELETE FROM )" + table + R"(
        WHERE id = (
            WITH PseudoIDs AS (
                SELECT ROW_NUMBER() OVER (ORDER BY id) AS pseudo_id, id
                FROM )" + table + R"(
            )
            SELECT id
            FROM PseudoIDs
            WHERE pseudo_id = ?
        );
    )";

	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
	}

	// Bind the pseudo-ID
	if (sqlite3_bind_int(stmt, 1, pseudoId) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		throw std::runtime_error("Failed to bind pseudo-ID: " + std::string(sqlite3_errmsg(db)));
	}

	// Execute the query
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		throw std::runtime_error("Failed to execute statement: " + std::string(sqlite3_errmsg(db)));
	}

	sqlite3_finalize(stmt);
}

db::Record db::Database::getRecord(std::string &table, RecordData &data) const {
	// Construct the SQL query with a WHERE clause
	std::string whereClause;
	for (const auto &key: data | std::views::keys) {
		if (!whereClause.empty()) {
			whereClause += " AND "; // Add AND between conditions
		}
		whereClause += key + " = ?";
	}

	const std::string query = std::format("SELECT * FROM {} WHERE {}", table, whereClause);

	// Prepare the SQLite statement
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
	}

	// Bind the parameters dynamically
	int index = 1;
	for (const auto &[key, value]: data) {
		if (std::holds_alternative<int>(value)) {
			sqlite3_bind_int(stmt, index, std::get<int>(value));
		} else if (std::holds_alternative<double>(value)) {
			sqlite3_bind_double(stmt, index, std::get<double>(value));
		} else if (std::holds_alternative<std::string>(value)) {
			sqlite3_bind_text(stmt, index, std::get<std::string>(value).c_str(), -1, SQLITE_STATIC);
		} else {
			sqlite3_finalize(stmt);
			throw std::runtime_error("Unsupported field type in data: " + key);
		}
		++index;
	}

	// Execute the SQL statement and fetch the single record
	RecordData recordData;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
			const std::string columnName = sqlite3_column_name(stmt, i);
			switch (sqlite3_column_type(stmt, i)) {
				case SQLITE_INTEGER:
					recordData[columnName] = sqlite3_column_int(stmt, i);
					break;
				case SQLITE_FLOAT:
					recordData[columnName] = sqlite3_column_double(stmt, i);
					break;
				case SQLITE_TEXT:
					recordData[columnName] = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)));
					break;
				case SQLITE_NULL:
					recordData[columnName] = ""; // Treat NULL as an empty string
					break;
				default:
					sqlite3_finalize(stmt);
					throw std::runtime_error("Unsupported column type for column: " + columnName);
			}
		}
	} else {
		sqlite3_finalize(stmt);
		throw std::runtime_error("Record not found with the given criteria");
	}

	Record record(recordData, table);
	// Finalize the statement
	sqlite3_finalize(stmt);

	return record;
}

db::Record db::Database::getRecordByPseudoId(const std::string &table, const int pseudoId) const {
	const std::string query = R"(
        SELECT * FROM )" + table + R"(
        WHERE id = (
            WITH PseudoIDs AS (
                SELECT ROW_NUMBER() OVER (ORDER BY id) AS pseudo_id, id
                FROM )" + table + R"(
            )
            SELECT id
            FROM PseudoIDs
            WHERE pseudo_id = ?
        );
    )";

	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
	}

	// Bind the pseudo-ID
	if (sqlite3_bind_int(stmt, 1, pseudoId) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		throw std::runtime_error("Failed to bind pseudo-ID: " + std::string(sqlite3_errmsg(db)));
	}

	// Execute the SQL statement and fetch the single record
	RecordData recordData;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
			const std::string columnName = sqlite3_column_name(stmt, i);
			switch (sqlite3_column_type(stmt, i)) {
				case SQLITE_INTEGER:
					recordData[columnName] = sqlite3_column_int(stmt, i);
					break;
				case SQLITE_FLOAT:
					recordData[columnName] = sqlite3_column_double(stmt, i);
					break;
				case SQLITE_TEXT:
					recordData[columnName] = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)));
					break;
				case SQLITE_NULL:
					recordData[columnName] = ""; // Treat NULL as an empty string
					break;
				default:
					sqlite3_finalize(stmt);
					throw std::runtime_error("Unsupported column type for column: " + columnName);
			}
		}
	} else {
		sqlite3_finalize(stmt);
		throw std::runtime_error("Record not found with the given criteria");
	}

	Record record(recordData, table);
	// Finalize the statement
	sqlite3_finalize(stmt);

	return record;
}

std::vector<db::Record> db::Database::getAllRecords(const std::string &table) const {
	const std::string query = std::format("SELECT * FROM {}", table);

	// Prepare the SQLite statement
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
	}

	std::vector<Record> records;

	// Execute the query and retrieve each row
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		RecordData data;
		for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
			const std::string column_name = sqlite3_column_name(stmt, i);

			// Handle column values based on SQLite's column type
			switch (sqlite3_column_type(stmt, i)) {
				case SQLITE_INTEGER:
					data[column_name] = sqlite3_column_int(stmt, i);
					break;
				case SQLITE_FLOAT:
					data[column_name] = sqlite3_column_double(stmt, i);
					break;
				case SQLITE_TEXT:
					data[column_name] = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)));
					break;
				case SQLITE_NULL:
					data[column_name] = ""; // Treat NULL as an empty string
					break;
				default:
					sqlite3_finalize(stmt);
					throw std::runtime_error("Unsupported column type for column: " + column_name);
			}
		}
		// Add the data to the records vector
		records.emplace_back(data, table);
	}

	// Finalize the statement and return results
	sqlite3_finalize(stmt);
	return records;
}
