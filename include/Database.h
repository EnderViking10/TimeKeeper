#pragma once
#include <sqlite3.h>
#include <variant>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace db {
	/*
	 * Example
	 *    Record record(1, {
	 *        {"name", "John Doe"},
	 *        {"age", 31},
	 *        {"height", 69.42}
	 *    });
	 *    The Field is "John Doe", 31, 69.42
	 *    The RecordData is the {"name", <Field>}
	 *    The Record is multiple RecordData
	 */
	using Field = std::variant<int, double, std::string>;
	using RecordData = std::unordered_map<std::string, Field>;

	/**
	 * @brief Represents a database record, including its data and corresponding table.
	 *
	 * This structure is used to encapsulate data associated with a database entry.
	 * It includes the table name and the record data as key-value pairs, which can
	 * be utilized for constructing and executing SQL queries for operations such
	 * as INSERT, SELECT, and DELETE.
	 */
	struct Record {
		RecordData data;
		std::string table;

		Record(RecordData data, std::string table) : data(std::move(data)), table(std::move(table)) {
		};
	};

	/**
	 * @brief Represents a column in a database table definition.
	 *
	 * This structure is used to define attributes of a database column, such as its name, data type,
	 * constraints (e.g., primary key, auto-increment, not null, unique), default value, and foreign key reference.
	 * It facilitates creating database schemas and specifying column properties when defining tables.
	 */
	struct Column {
		std::string name;
		std::string type;
		bool primaryKey = false;
		bool autoIncrement = false;
		bool notNull = false;
		bool unique = false;
		std::optional<std::string> defaultVal = std::nullopt;
		std::optional<std::string> foreignKey = std::nullopt;
	};

	class Database {
	public:
		explicit Database(std::string db_path) : db_path(std::move(db_path)) {
			openDatabase();
		};

		~Database() {
			closeDatabase();
		};

		/**
		 * @brief Creates a new table in the database with the specified columns.
		 *
		 * This method constructs and executes a SQL CREATE TABLE statement with the
		 * specified table name and column definitions. Column attributes such as
		 * primary key, auto-increment, not-null, unique, default values, and foreign
		 * key constraints are supported.
		 *
		 * @param table The name of the table to create.
		 * @param columns A vector of Column objects defining the structure and
		 *        attributes of the table's columns. Each Column object must provide
		 *        details such as column name, type, and any constraints (e.g.,
		 *        primary key, default value, etc.).
		 *
		 * @throw std::invalid_argument Thrown if the columns vector is empty.
		 * @throw std::runtime_error Thrown if the SQLite statement preparation or
		 *        execution fails.
		 */
		void createTable(const std::string &table, const std::vector<db::Column> &columns) const;

		/**
		 * @brief Adds a new record to the database.
		 *
		 * This method constructs an SQL INSERT query based on the provided record's
		 * data and inserts the data into the database. Throws an exception if
		 * the query preparation or execution fails.
		 *
		 * @param record The record to be inserted into the database. It contains
		 * the table name and key-value pairs of column names and values.
		 *
		 * @throw std::runtime_error If the SQL statement preparation, value binding,
		 * or execution fails.
		 */
		void addRecord(const Record &record) const;

		/**
		 * Removes a record from the specified table in the database.
		 *
		 * Constructs a DELETE SQL query based on the provided table name and
		 * condition data. Binds the values dynamically before executing the query.
		 *
		 * @param table The name of the table from which the record should be removed.
		 * @param data The condition set for identifying the record to delete.
		 *             The keys represent column names, and the corresponding values
		 *             represent the conditions for deletion. Supports int, double,
		 *             and string as data types.
		 * @throws std::runtime_error If the SQL statement preparation or execution fails,
		 *                            or if unsupported field types are encountered.
		 */
		void removeRecord(const std::string &table, const RecordData &data) const;

		/**
		 * Removes a record from the specified database table using a pseudo-ID. A pseudo-ID
		 * is a sequential number mapped to a record, based on the row number of its ID when
		 * the table is ordered by ID.
		 *
		 * @param table The name of the database table where the record should be removed.
		 * @param pseudoId The sequential pseudo-ID representing the position of the
		 *        record to delete, based on the order of its ID in the table.
		 *
		 * @throw std::runtime_error If the query preparation, binding, or execution fails,
		 *        an exception is thrown with the relevant SQLite error message.
		 */
		void removeRecordByPseudoId(const std::string &table, int pseudoId) const;

		/**
		 * Retrieves a record from the specified table in the database that matches the given criteria.
		 *
		 * This method constructs a SQL query with a WHERE clause using the provided table name and criteria
		 * and retrieves a single record that matches the conditions. If no record is found or an error occurs,
		 * this method throws an exception.
		 *
		 * @param table The name of the database table to search.
		 * @param data The key-value pairs representing the conditions for the WHERE clause of the SQL query.
		 * Each key is a column name, and the value is the expected value for that column.
		 * Supported value types include int, double, and std::string.
		 *
		 * @return A Record object containing the data of the matched row from the database table.
		 * @throw std::runtime_error if the SQL statement preparation or execution fails,
		 * if unsupported field types are used in data, or if no matching record is found.
		 */
		Record getRecord(std::string &table, RecordData &data) const;

		/**
		 * Retrieves a record from the database table using a pseudo-ID.
		 * A pseudo-ID is a row number assigned based on the order of entries in the table.
		 *
		 * @param table The name of the table from which the record is to be retrieved.
		 * @param pseudoId The pseudo-ID of the record to retrieve. This is a 1-based index of the row in the table.
		 * @return The record associated with the given pseudo-ID.
		 * @throws std::runtime_error If the SQL statement preparation, binding, or execution fails,
		 *                            or if no record is found for the given pseudo-ID.
		 */
		Record getRecordByPseudoId(const std::string &table, int pseudoId) const;

		/**
		 * Retrieves all records from the specified table in the database.
		 *
		 * Constructs a query to select all rows from the given table and executes it.
		 * Each row is converted into a Record object, and all such objects are
		 * returned in a vector.
		 *
		 * @param table The name of the table from which to retrieve all records.
		 * @return A vector containing all records from the specified table. Each record
		 *         is represented as a Record object.
		 * @throws std::runtime_error If the SQL statement preparation or execution fails
		 *         or if an unsupported column type is encountered.
		 */
		std::vector<Record> getAllRecords(const std::string &table) const;

	private:
		std::string db_path;
		sqlite3 *db{};

		/**
		 * Opens a connection to the SQLite database using the file path stored in the `db_path` member.
		 *
		 * This method initializes the SQLite database handle and establishes a connection to the database file.
		 * If the connection cannot be established, an exception is thrown with the error message provided
		 * by SQLite.
		 *
		 * @throws std::runtime_error If the database connection cannot be established.
		 */
		void openDatabase();

		/**
		 * Closes the connection to the currently opened SQLite database.
		 *
		 * This method attempts to close the SQLite database handle associated with the database.
		 * If the closure fails, an exception is thrown with the relevant error message provided by SQLite.
		 *
		 * @throws std::runtime_error If the database cannot be successfully closed.
		 */
		void closeDatabase() const;

		/**
		 * Creates a SQL SELECT query for a specified table and record data.
		 * You can give it a record like RecordData("id", 1) or RecordData("name", "bob")
		 *
		 * @param table The name of the database table to query.
		 * @param data A RecordData object containing the key-value pairs used as conditions in the WHERE clause.
		 * @return A string representing the SQL SELECT query with placeholders for parameterized queries.
		 */
		static std::string createSelectQuery(const std::string &table, const RecordData &data);
	};
}
