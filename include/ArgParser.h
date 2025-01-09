#pragma once
#include <optional>
#include <string>
#include <utility>
#include <vector>

/*
 * Everything is referred to as an arg.
 * Options, positional arguments, non-positional arguments. All arg/arguments
 *
 * Comes with built-in help command. To use where ever you do your arg handling just handle this as well.
 */
namespace tike {
	/**
	 * @struct Arg
	 * @brief Represents a command-line argument definition.
	 *
	 * This structure is used to define the properties of a command-line argument
	 * including its name, type, description, and whether it is required or optional.
	 * It supports both long-form and short-form argument names and optionally allows
	 * a value to be associated with the argument.
	 *
	 * The `Arg` structure is primarily used within argument-parsing utilities to
	 * define and process command-line arguments.
	 *
	 * @param name The long-form name of the argument. Used with a double dash (e.g., `--name`).
	 * @param shortName An optional short-form name for the argument. Used with a single dash (e.g., `-n`).
	 * @param value An optional value associated with the argument. May contain the argument's value after parsing.
	 * @param type Specifies the type of the argument (e.g., "flag", "string"). Defines how the argument is processed.
	 * @param description A brief description of the argument, used for generating help or usage information.
	 * @param required Indicates whether this argument is mandatory. Defaults to `false`.
	 */
	struct Arg {
		std::string name;
		std::optional<std::string> shortName;
		std::optional<std::string> value;
		std::string type;
		std::string description;
		bool required;

		explicit Arg(std::string name, std::optional<std::string> shortName, std::string type,
					 std::string description = "Default argument description", const bool required = false) {
			this->name = std::move(name);
			this->shortName = std::move(shortName);
			this->value = std::nullopt;
			this->type = std::move(type);
			this->description = std::move(description);
			this->required = required;
		}
	};

	class ArgParser {
	public:
		/**
		 * @class ArgParser
		 * @brief Parses and processes command-line arguments for a program.
		 *
		 * The ArgParser class provides a convenient interface to define, parse, and manage
		 * command-line arguments for a program. It allows users to add custom arguments and
		 * automatically handles a `--help` or `-h` flag to display usage information, unless disabled.
		 *
		 * This constructor initializes the parser with basic program information and
		 * optionally adds a default help argument. If `customHelp` is set to `true`,
		 * the help argument will not be automatically added, allowing custom help functionality.
		 *
		 * @param program The name of the program. Typically displayed in usage/help information.
		 * @param description A short description of what the program does.
		 * @param customHelp If set to `true`, the default help argument is not added.
		 */
		ArgParser(std::string program, std::string description, const bool customHelp = false)
			: program(std::move(program)), description(std::move(description)) {
			if (!customHelp) {
				addArg(Arg("help", "h", "flag", "Show this help page"));
			}
		};

		/**
		 * @brief Parses the command-line arguments according to the defined argument specifications.
		 *
		 * This method interprets the provided `argc` and `argv[]` to match against the predefined
		 * list of arguments stored in the `args` object. It supports both long-form (`--argument`)
		 * and short-form (`-a`) argument styles, as well as flag-style arguments and those requiring
		 * associated values.
		 *
		 * Key features include:
		 * - Identification and handling of long-form and short-form argument names.
		 * - Handling required arguments and ensuring they are provided by the user.
		 * - Throwing exceptions for unknown arguments, missing values, or unexpected positional arguments.
		 * - Associating parsed values with their respective argument definitions in the `args` list.
		 *
		 * @param argc The count of arguments supplied to the program, including the program name itself.
		 * @param argv An array of null-terminated character strings representing the arguments (including the program name).
		 *
		 * @throws std::invalid_argument If an unrecognized argument is encountered, or if required arguments or values are missing.
		 */
		void parse(int argc, const char *argv[]);

		/**
		 * @brief Adds a command-line argument to the argument parser.
		 *
		 * This method appends a new argument definition to the internal list of arguments
		 * handled by the parser. The argument will be considered during parsing.
		 *
		 * @param arg An object of type Arg representing the argument to be added.
		 *            It includes the details such as name, shorthand, type, description,
		 *            and whether it is required or optional.
		 */
		void addArg(const Arg &arg);

		/**
		 * Checks if an argument with the specified name has a specific value.
		 *
		 * @param name The name of the argument to be checked.
		 * @return True if an argument with the specified name exists, has a value; otherwise, false.
		 */
		bool argHasValue(const std::string &name);

		/**
		 * @brief Retrieves an argument by its name.
		 *
		 * This method retrieves a reference to the `Arg` object in the `args` vector
		 * that matches the specified argument name. Throws an exception if no matching
		 * argument is found.
		 *
		 * @param name The name of the argument to retrieve.
		 * @return A reference to the matching `Arg` object.
		 * @throws std::invalid_argument If the specified argument name doesn't exist.
		 */
		[[nodiscard]] const Arg &getArgByName(const std::string &name) const;

		void helpCommand() const;

	private:
		std::vector<Arg> args;
		std::string program;
		std::string description;
	};
} // namespace tike
