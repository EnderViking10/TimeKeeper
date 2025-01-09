#include "ArgParser.h"

#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <bits/ranges_algo.h>

void tike::ArgParser::addArg(const Arg &arg) {
	args.push_back(arg);
}

void tike::ArgParser::parse(const int argc, const char *argv[]) {
	// Loop through the arguments
	for (int index = 1; index < argc; index++) {
		std::string currentArg = argv[index];

		// First checks if it has the -
		if (currentArg[0] == '-') {
			bool matched = false;

			// Checks if it is -- with no argument name
			if (currentArg == "--") {
				throw std::invalid_argument("Unexpected `--` without argument.");
			}
			// Checks if it is --, but has a name
			if (currentArg[1] == '-') {
				// The 3rd char and beyond in the currentArg string
				const std::string longName = currentArg.substr(2);
				// Loop through the list of args in the object
				for (auto &arg: args) {
					// If the .name parameter matches the arg name
					if (arg.name == longName) {
						matched = true;
						if (arg.type == "flag") {
							arg.value = "true";
						} else if (index + 1 < argc) {
							arg.value = argv[++index];
						} else {
							throw std::invalid_argument("Missing value for argument: --" + longName);
						}
						break;
					}
				}
			} else {
				// If not -- then it's just -
				// The 2nd char and beyond in currentArg string
				const std::string shortName = currentArg.substr(1);
				// Loop through the list of args in the object
				for (auto &arg: args) {
					// If the .name parameter matches the arg name
					if (arg.shortName == shortName) {
						matched = true;
						if (arg.type == "flag") {
							arg.value = "true";
						} else if (index + 1 < argc) {
							arg.value = argv[++index];
						} else {
							throw std::invalid_argument("Missing value for argument: -" + shortName);
						}
						break;
					}
				}
			}

			if (!matched) {
				throw std::invalid_argument("Unknown argument: " + currentArg);
			}
		} else {
			throw std::invalid_argument("Unexpected positional argument: " + currentArg);
		}
	}
	// Checks if any required args were not supplied
	for (const auto &arg: args) {
		if (arg.required && !arg.value) {
			throw std::invalid_argument("Missing required argument: --" + arg.name);
		}
	}
}

bool tike::ArgParser::argHasValue(const std::string &name) {
	return std::ranges::any_of(args, [&](const Arg &arg) {
		return arg.name == name && arg.value.has_value();
	});
}

const tike::Arg &tike::ArgParser::getArgByName(const std::string &name) const {
	// Use std::ranges::find_if to find the argument by name
	const auto it = std::ranges::find_if(args, [&](const Arg &arg) {
		return arg.name == name;
	});

	// Check if the iterator reached the end (no matching argument found)
	if (it == args.end()) {
		throw std::invalid_argument("Argument not found with name: --" + name);
	}

	// Return a valid reference to the Arg object
	return *it;
}

void tike::ArgParser::helpCommand() const {
    // Display Usage
    std::cout << "Usage: " << program << " [OPTIONS]" << std::endl;
    std::cout << std::endl;

    // Display Description
    if (!description.empty()) {
        std::cout << description << std::endl << std::endl;
    }

    // Display Options Header
    std::cout << "Options:" << std::endl;

    // Sort arguments by name (long name, `name` field)
    std::vector<Arg> sortedArgs = args; // Copy args to avoid modifying the original vector
    std::ranges::sort(sortedArgs.begin(), sortedArgs.end(), [](const Arg &a, const Arg &b) {
        return a.name < b.name; // Sort lexicographically by `name`
    });

    // Find the longest option name (for alignment)
    int maxOptionLength = 0;
    for (const auto &arg : sortedArgs) {
        std::ostringstream optionStream;

        // Short name
        if (arg.shortName.has_value()) {
            optionStream << "-" << arg.shortName.value() << ", ";
        } else {
            optionStream << "    "; // Spaces if short name is absent
        }

        // Long name
        optionStream << "--" << arg.name;

        // Update the maximum length
        maxOptionLength = std::max(maxOptionLength, static_cast<int>(optionStream.str().length()));
    }

    // Adjust spacing for descriptions
	constexpr int padding = 6; // Space between option and description
    const int descriptionStart = maxOptionLength + padding;

    // Display sorted arguments
    for (const auto &arg : sortedArgs) {
        std::ostringstream optionStream;

        // Short name
        if (arg.shortName.has_value()) {
            optionStream << "    -" << arg.shortName.value() << ", ";
        } else {
            optionStream << "        "; // Spaces if short name is absent;
        }

        // Long name
        optionStream << "--" << arg.name;

        // Print the option names and align description
        std::cout << std::left << std::setw(descriptionStart)
                  << optionStream.str();

        // Print the description
        std::cout << arg.description << std::endl;
    }
}