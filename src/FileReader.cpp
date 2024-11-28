#include "FileReader.hpp"

// Internal trim function 
std::string FileReader::trimInternal(const std::string& str) {
    std::string trimmed = str;
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), trimmed.end());
    return trimmed;
}

EpitrendBinaryFormat FileReader::parseEpitrendFile(
    int year, 
    int month, 
    int day, 
    int hour, 
    bool verbose
) {
    // Array for month names
    const std::string MONTH_NAMES[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    // Validate month input
    if (month < 1 || month > 12) {
        throw std::invalid_argument("Invalid month: " + std::to_string(month));
    }

    // Construct the file path dynamically
    std::ostringstream oss;
    oss << Config::getDataDir()
        << std::setfill('0') << year << "/"
        << std::setw(2) << month << "-" << MONTH_NAMES[month - 1] << "/"
        << std::setw(2) << day << "day-" << std::setw(2) << hour << "hr.txt";
    std::string fullpath = oss.str();

    if (verbose) {
        std::cout << "Opening file: " << fullpath << "\n";
    }

    // Open the file
    std::ifstream file(fullpath);
    if (!file.is_open()) {
        throw std::runtime_error("Error parseEpitrendFile function call: Could not open file: " + fullpath);
    }

    EpitrendBinaryFormat binaryFormat;
    std::string line;
    int lineNumber = 0;

    // Process each line
    while (std::getline(file, line)) {
        lineNumber++;
        line = trimInternal(line);

        if (line.empty()) {
            continue; // Skip empty lines
        }

        if (verbose) {
            std::cout << "Line " << lineNumber << ": " << line << "\n";
        }

        // Parse key-value pairs
        if (line.find("=") == std::string::npos) {
            continue; // Skip malformed lines
        }

        std::vector<std::string> lineTokens = split(line, "=");
        if (lineTokens.size() < 2) {
            if (verbose) {
                std::cerr << "Malformed line at " << lineNumber << ": " << line << "\n";
            }
            continue;
        }

        const std::string& key = lineTokens[0];
        const std::string& value = lineTokens[1];

        try {
            if (key == "TotalDataItems") {
                binaryFormat.setTotalDataItems(std::stoi(value));
                if (verbose) {
                    std::cout << "Set TotalDataItems: " << value << "\n";
                }
            } else if (key == "Misc") {
                std::vector<std::string> miscTokens = split(value, ";");
                if (!miscTokens.empty()) {
                    std::vector<std::string> timeResTokens = split(miscTokens[0], ":");
                    if (timeResTokens.size() == 2) {
                        binaryFormat.setTimeResolution(std::stod(timeResTokens[1]));
                        if (verbose) {
                            std::cout << "Set TimeResolution: " << timeResTokens[1] << "\n";
                        }
                    }
                }
            } else if (key == "CurrentDay") {
                binaryFormat.setCurrentDay(std::stoi(value));
                if (verbose) {
                    std::cout << "Set CurrentDay: " << value << "\n";
                }
            } else if (key == "DataItem") {
                EpitrendBinaryFormat::DataItem dataItem;
                std::vector<std::string> dataItemTokens = split(value, ";");

                for (const auto& token : dataItemTokens) {
                    std::vector<std::string> dataItemKeyValue = split(token, ":");
                    if (dataItemKeyValue.size() == 2) {
                        const std::string& dataKey = dataItemKeyValue[0];
                        const std::string& dataValue = dataItemKeyValue[1];

                        if (dataKey == "Name") {
                            dataItem.Name = dataValue;
                        } else if (dataKey == "Type") {
                            dataItem.Type = dataValue;
                        } else if (dataKey == "Range") {
                            dataItem.Range = dataValue;
                        } else if (dataKey == "TotalValues") {
                            dataItem.TotalValues = std::stoi(dataValue);
                        } else if (dataKey == "ValueOffset") {
                            dataItem.ValueOffset = std::stoi(dataValue);
                        }
                    }
                }

                if (!dataItem.Name.empty()) {
                    binaryFormat.addDataItem(dataItem.Name, dataItem);
                    if (verbose) {
                        std::cout << "Added DataItem: " << dataItem.Name << "\n";
                    }
                }
            }
        } catch (const std::exception& e) {
            if (verbose) {
                std::cerr << "Error parsing line " << lineNumber << ": " << e.what() << "\n";
            }
            continue;
        }
    }

    file.close();

    // Final summary
    if (verbose) {
        binaryFormat.printSummary();
    }

    return binaryFormat;
}