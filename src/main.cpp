#include <ArgParser.h>
#include <Database.h>
#include <iostream>
#include <ranges>
#include <chrono>

#define VERSION_NUMBER "1.0.0"
#define VERSION_NAME "Ymir"

std::string getHomeDir() {
#ifdef _WIN32
	// On Windows, use USERPROFILE
	const char* homeDir = std::getenv("USERPROFILE");
#else
	// On Linux/macOS, use HOME
	const char *homeDir = std::getenv("HOME");
#endif
	return homeDir ? std::string(homeDir) : "";
}

int main(const int argc, const char *argv[]) {
	// Create db and make sure tables exist
	const std::string homeDir = getHomeDir();
	const db::Database db(homeDir + "/.tike.db");
	db.createTable("tasks", {
					   db::Column{.name = "id", .type = "INTEGER", .primaryKey = true, .autoIncrement = true},
					   db::Column{.name = "title", .type = "TEXT"},
					   db::Column{.name = "description", .type = "TEXT"},
					   db::Column{.name = "timeCreated", .type = "DATETIME", .defaultVal = "CURRENT_TIMESTAMP"}
				   });
	db.createTable("completedTasks", {
					   db::Column{.name = "id", .type = "INTEGER", .primaryKey = true},
					   db::Column{.name = "title", .type = "TEXT"},
					   db::Column{.name = "description", .type = "TEXT"},
					   db::Column{.name = "timeCreated", .type = "DATETIME"},
					   db::Column{.name = "timeCompleted", .type = "DATETIME", .defaultVal = "CURRENT_TIMESTAMP"}
				   });

	// Set up parser
	tike::ArgParser parser("Tike", "TimeKeeper");
	try {
		parser.addArg(tike::Arg("add", "a", "flag", "Add a new task"));
		parser.addArg(tike::Arg("complete", "c", "int", "Mark a task as completed by id"));
		parser.addArg(tike::Arg("description", "d", "string", "Description of the task"));
		parser.addArg(tike::Arg("list", "l", "int", "List a task by id"));
		parser.addArg(tike::Arg("list-all", "L", "flag", "List all tasks"));
		parser.addArg(tike::Arg("list-all-completed", std::nullopt, "flag", "List all completed tasks"));
		parser.addArg(tike::Arg("list-completed", std::nullopt, "int", "List a completed task by id"));
		parser.addArg(tike::Arg("remove", "r", "int", "Remove a task by id"));
		parser.addArg(tike::Arg("title", "t", "string", "Title of the task"));
		parser.addArg(tike::Arg("version", "v", "flag", "Prints the version number"));
		parser.parse(argc, argv);
	} catch (const std::invalid_argument &error) {
		std::cerr << "Error: " << error.what() << std::endl;
		exit(1);
	} catch (const std::exception &error) {
		std::cerr << "Unhandled exception: " << error.what() << std::endl;
		exit(1);
	} catch (...) {
		std::cerr << "Unknown error occurred" << std::endl;
		exit(1);
	}

	try {
		if (parser.argHasValue("help")) {
			parser.helpCommand();
			exit(0);
		}
		if (parser.argHasValue("version")) {
			std::cout << "TimeKeeper version " << VERSION_NAME << " (" << VERSION_NUMBER << ")" << std::endl;
			exit(0);
		}
		if (parser.argHasValue("add")) {
			if (!parser.argHasValue("title")) {
				throw std::invalid_argument("Missing required argument: --title");
			}

			db::RecordData data = {
				{"title", parser.getArgByName("title").value.value()}
			};
			if (parser.argHasValue("description")) {
				data["description"] = parser.getArgByName("description").value.value();
			}
			const db::Record record(data, "tasks");

			db.addRecord(record);

			std::cout << "Task added successfully" << std::endl;
			exit(0);
		}
		if (parser.argHasValue("list")) {
			std::string table = "tasks";
			db::RecordData data;
			const db::Record record = db.getRecordByPseudoId(
				table, std::stoi(parser.getArgByName("list").value.value()));

			if (record.data.empty()) {
				std::cout << "Task not found: " << "\n";
				exit(1);
			}

			std::vector<db::Record> records = db.getAllRecords(table);

			int taskNumber = 1;
			for ([[maybe_unused]] const auto &i: records) {
				if (i.data.at("id") == record.data.at("id")) { break; }
				taskNumber++;
			}

			// Print header row with columns lined up
			constexpr int columnWidth = 20; // Adjust as needed for your data
			std::cout << "Task:\n";
			std::cout << std::left << std::setw(5) << "#" // Task Number
					<< std::setw(columnWidth) << "Task Title" // Title column
					<< std::setw(columnWidth) << "Task Description" // Description column
					<< std::setw(columnWidth) << "Time Created (UTC)" // Time Created column
					<< "\n";
			std::cout << std::string(5 + 3 * columnWidth, '-') << "\n"; // Divider

			// Extract columns (title, description, timeCreated)
			std::string title, description, timeCreated;

			// Search for "title", "description", and "timeCreated" in the record data
			title = std::get<std::string>(record.data.at("title"));
			description = std::get<std::string>(record.data.at("description"));
			timeCreated = std::get<std::string>(record.data.at("timeCreated"));

			// Print task row with columns aligned
			std::cout << std::left << std::setw(5) << taskNumber // Task Number
					<< std::setw(columnWidth) << title // Task Title
					<< std::setw(columnWidth) << description // Task Description
					<< std::setw(columnWidth) << timeCreated // Time Created
					<< std::endl;
			exit(0);
		}
		if (parser.argHasValue("list-all")) {
			const std::string table = "tasks";
			// Get all records from the table
			std::vector<db::Record> records = db.getAllRecords(table);

			// Check if there are no records
			if (records.empty()) {
				std::cout << "No tasks found in table: " << table << "\n";
				exit(1);
			}

			// Print header row with columns lined up
			constexpr int columnWidth = 20; // Adjust as needed for your data
			std::cout << "Tasks:\n";
			std::cout << std::left << std::setw(5) << "#" // Task Number
					<< std::setw(columnWidth) << "Task Title" // Title column
					<< std::setw(columnWidth) << "Task Description" // Description column
					<< std::setw(columnWidth) << "Time Created (UTC)" // Time Created column
					<< std::endl;
			std::cout << std::string(5 + 3 * columnWidth, '-') << std::endl; // Divider

			// Print each record
			int taskNumber = 1;
			for (const auto &record: records) {
				// Extract columns (title, description, timeCreated)
				std::string title, description, timeCreated;

				// Search for "title", "description", and "timeCreated" in the record data
				title = std::get<std::string>(record.data.at("title"));
				description = std::get<std::string>(record.data.at("description"));
				timeCreated = std::get<std::string>(record.data.at("timeCreated"));

				// Print task row with columns aligned
				std::cout << std::left << std::setw(5) << taskNumber++ // Task Number
						<< std::setw(columnWidth) << title // Task Title
						<< std::setw(columnWidth) << description // Task Description
						<< std::setw(columnWidth) << timeCreated // Time Created
						<< std::endl;
			}
		}
		if (parser.argHasValue("remove")) {
			std::string table = "tasks";
			int id = std::stoi(parser.getArgByName("remove").value.value());
			db.removeRecordByPseudoId(table, id);

			std::cout << "Task " << parser.getArgByName("remove").value.value() << " removed successfully" << std::endl;
		}
		if (parser.argHasValue("complete")) {
			std::string notCompletedTable = "tasks";
			std::string completedTable = "completedTasks";
			int id = std::stoi(parser.getArgByName("complete").value.value());

			// Get the not completed record
			db::Record notCompletedRecord = db.getRecordByPseudoId(notCompletedTable, id);
			// Transfer to new Record
			db::RecordData completedData;
			completedData["title"] = notCompletedRecord.data.at("title");
			completedData["description"] = notCompletedRecord.data.at("description");
			completedData["timeCreated"] = notCompletedRecord.data.at("timeCreated");
			db::Record completedRecord(completedData, completedTable);

			// Add it to the completed table
			db.addRecord(completedRecord);

			// Remove it from not completed table
			db.removeRecord(notCompletedTable, notCompletedRecord.data);
		}
		if (parser.argHasValue("list-completed")) {
			std::string table = "completedTasks";
			db::RecordData data;
			const db::Record record = db.getRecordByPseudoId(
				table, std::stoi(parser.getArgByName("list-completed").value.value()));

			if (record.data.empty()) {
				std::cout << "Task not found: " << "\n";
				exit(1);
			}

			std::vector<db::Record> records = db.getAllRecords(table);

			int taskNumber = 1;
			for ([[maybe_unused]] const auto &i: records) {
				if (i.data.at("id") == record.data.at("id")) { break; }
				taskNumber++;
			}

			// Print header row with columns lined up
			constexpr int columnWidth = 20; // Adjust as needed for your data
			std::cout << "Task:\n";
			std::cout << std::left << std::setw(5) << "#" // Task Number
					<< std::setw(columnWidth) << "Task Title" // Title column
					<< std::setw(columnWidth) << "Task Description" // Description column
					<< std::setw(columnWidth) << "Time Created (UTC)" // Time Created column
					<< "\n";
			std::cout << std::string(5 + 3 * columnWidth, '-') << "\n"; // Divider

			// Extract columns (title, description, timeCreated)
			std::string title, description, timeCreated;

			// Search for "title", "description", and "timeCreated" in the record data
			title = std::get<std::string>(record.data.at("title"));
			description = std::get<std::string>(record.data.at("description"));
			timeCreated = std::get<std::string>(record.data.at("timeCreated"));

			// Print task row with columns aligned
			std::cout << std::left << std::setw(5) << taskNumber // Task Number
					<< std::setw(columnWidth) << title // Task Title
					<< std::setw(columnWidth) << description // Task Description
					<< std::setw(columnWidth) << timeCreated // Time Created
					<< std::endl;
			exit(0);
		}
		if (parser.argHasValue("list-all-completed")) {
			const std::string table = "completedTasks";
			// Get all records from the table
			std::vector<db::Record> records = db.getAllRecords(table);

			// Check if there are no records
			if (records.empty()) {
				std::cout << "No tasks found in table: " << table << "\n";
				exit(1);
			}

			// Print header row with columns lined up
			constexpr int columnWidth = 20; // Adjust as needed for your data
			std::cout << "Tasks:\n";
			std::cout << std::left << std::setw(5) << "#" // Task Number
					<< std::setw(columnWidth) << "Task Title" // Title column
					<< std::setw(columnWidth) << "Task Description" // Description column
					<< std::setw(columnWidth) << "Time Created (UTC)" // Time Created column
					<< std::endl;
			std::cout << std::string(5 + 3 * columnWidth, '-') << std::endl; // Divider

			// Print each record
			int taskNumber = 1;
			for (const auto &record: records) {
				// Extract columns (title, description, timeCreated)
				std::string title, description, timeCreated;

				// Search for "title", "description", and "timeCreated" in the record data
				title = std::get<std::string>(record.data.at("title"));
				description = std::get<std::string>(record.data.at("description"));
				timeCreated = std::get<std::string>(record.data.at("timeCreated"));

				// Print task row with columns aligned
				std::cout << std::left << std::setw(5) << taskNumber++ // Task Number
						<< std::setw(columnWidth) << title // Task Title
						<< std::setw(columnWidth) << description // Task Description
						<< std::setw(columnWidth) << timeCreated // Time Created
						<< std::endl;
			}
		}
	} catch (const std::invalid_argument &error) {
		std::cerr << "Error: " << error.what() << std::endl;
		exit(1);
	} catch (const std::exception &error) {
		std::cerr << "Unhandled exception: " << error.what() << std::endl;
		exit(1);
	} catch (...) {
		std::cerr << "Unknown error occurred" << std::endl;
		exit(1);
	}

	return 0;
}
