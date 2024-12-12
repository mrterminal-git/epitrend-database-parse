#include "FileReader.hpp"

namespace fs = std::filesystem;

// Internal trim function 
std::string FileReader::trimInternal(const std::string& str) {
    std::string trimmed = str;
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), trimmed.end());
    return trimmed;
}

// Parse the Epitrend binary format file
EpitrendBinaryFormat FileReader::parseEpitrendBinaryFormatFile(
    std::string GM,
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
    oss << Config::getDataDir() << GM << " Molly/"
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

    //

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

// Parse the Epitrend binary data file
void FileReader::parseEpitrendBinaryDataFile(
    EpitrendBinaryData& binary_data,
    std::string GM,
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

    // Parse the Epitrend binary format object
    EpitrendBinaryFormat binary_format = 
        FileReader::parseEpitrendBinaryFormatFile(GM, year, month, day, hour, verbose);

    // Construct the file path dynamically
    std::ostringstream oss;
    oss << Config::getDataDir() << GM << " Molly/"
        << std::setfill('0') << year << "/"
        << std::setw(2) << month << "-" << MONTH_NAMES[month - 1] << "/"
        << std::setw(2) << day << "day-" << std::setw(2) << hour << "hr-binary.txt";
    std::string fullpath = oss.str();

    if (verbose) {
        std::cout << "Opening file: " << fullpath << "\n";
    }

    // Open the file
    std::ifstream file(fullpath);
    if (!file.is_open()) {
        throw std::runtime_error("Error parseEpitrendBinaryDataFile function call: Could not open file: " + fullpath);
    }

    // Load binary data into a buffer
    std::vector<double> binary_buffer;
    float value;

    // Read the binary data
    while (file.read(reinterpret_cast<char*>(&value), sizeof(value))) {
        binary_buffer.push_back(value);
    }

    // Loop through all the data item names
    for(const std::string& data_item_name : binary_format.getAllDataItemNames()) {
        // For each name, get the time data and
        // Index the binary file from the offset to the total number of items in each data items            
        EpitrendBinaryFormat::DataItem current_data_item = binary_format.getDataItem(data_item_name);
        int start_index = current_data_item.ValueOffset;
        int end_index = current_data_item.TotalValues + start_index;
        
        // Looping through the time,value pairs for current date item name
        for(int i = (start_index+1) * 2; i < (end_index+1) * 2; i+=2) { 
            double current_time = static_cast<double>(binary_format.getCurrentDay()) + binary_buffer.at(i);
            binary_data.addDataItem(data_item_name, {current_time, (double) binary_buffer.at(i+1)}, verbose);

        }

    }

}

// Parse the server Epitrend binary format file
EpitrendBinaryFormat FileReader::parseServerEpitrendBinaryFormatFile(
    std::string GM,
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
    oss << Config::getServerEpitrendDataDir() << GM << " Molly/"
        << "EpiTrend/EpiTrendData/"
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
        throw std::runtime_error("Error parseServerEpitrendBinaryFormatFile function call: Could not open file: " + fullpath);
    }

    EpitrendBinaryFormat binaryFormat;
    std::string line;
    int lineNumber = 0;

    //

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

// Parse the Epitrend binary data file
void FileReader::parseServerEpitrendBinaryDataFile(
    EpitrendBinaryData& binary_data,
    std::string GM,
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

    // Parse the Epitrend binary format object
    EpitrendBinaryFormat binary_format = 
        FileReader::parseServerEpitrendBinaryFormatFile(GM, year, month, day, hour, verbose);

    // Construct the file path dynamically
    std::ostringstream oss;
    oss << Config::getServerEpitrendDataDir() << GM << " Molly/"
        << "EpiTrend/EpiTrendData/"
        << std::setfill('0') << year << "/"
        << std::setw(2) << month << "-" << MONTH_NAMES[month - 1] << "/"
        << std::setw(2) << day << "day-" << std::setw(2) << hour << "hr-binary.txt";
    std::string fullpath = oss.str();

    if (verbose) {
        std::cout << "Opening file: " << fullpath << "\n";
    }

    // Open the file
    std::ifstream file(fullpath);
    if (!file.is_open()) {
        throw std::runtime_error("Error parseServerEpitrendBinaryDataFile function call: Could not open file: " + fullpath);
    }

    // Load binary data into a buffer
    std::vector<double> binary_buffer;
    float value;

    // Read the binary data
    while (file.read(reinterpret_cast<char*>(&value), sizeof(value))) {
        binary_buffer.push_back(value);
    }

    // Loop through all the data item names
    for(const std::string& data_item_name : binary_format.getAllDataItemNames()) {
        // For each name, get the time data and
        // Index the binary file from the offset to the total number of items in each data items            
        EpitrendBinaryFormat::DataItem current_data_item = binary_format.getDataItem(data_item_name);
        int start_index = current_data_item.ValueOffset;
        int end_index = current_data_item.TotalValues + start_index;
        
        // Looping through the time,value pairs for current date item name
        for(int i = (start_index+1) * 2; i < (end_index+1) * 2; i+=2) { 
            double current_time = static_cast<double>(binary_format.getCurrentDay()) + binary_buffer.at(i);
            binary_data.addDataItem(data_item_name, {current_time, (double) binary_buffer.at(i+1)}, verbose);

        }

    }

}


// Parse the RGA data file
void FileReader::parseRGADataFile(
    RGAData& rga_data,
    std::string GM,
    int year,
    int month,
    int day,
    bool verbose
) {
    // Construct the directory path
    std::ostringstream dir_oss;
    dir_oss << "data/MBE1/"
            << std::setfill('0') << std::setw(4) << year << "-"
            << std::setw(3) << month << "-"
            << std::setw(2) << day;
    std::string directory = dir_oss.str();

    // Check if the directory exists
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        throw std::runtime_error("Error parseRGADataFile function call:"
        " Directory does not exist: " 
        + directory);
    }

    // Construct the regex pattern dynamically
    std::ostringstream oss;
    oss << "daily log, " << GM << ", RGA MPH .*?, "
        << std::setfill('0') << std::setw(4) << year << "-"
        << std::setw(3) << month << "-"
        << std::setw(2) << day << " .*?\\.dat";
    std::string pattern = oss.str();

    if (verbose) {
        std::cout << "In parseRGADataFile call: constructed directory path: " << directory << std::endl;
        std::cout << "In parseRGADataFile call: constructed regex pattern: " << pattern << std::endl;
    }

    // Compile the regex pattern
    std::regex regex_pattern(pattern);

    // Search for matching files
    bool file_found = false;

    // Storage structure for all lines in file
    std::vector<std::string> lines;


    if (verbose) 
        std::cout << "In parseRGADataFile call: searching for"
        " matching files in directory: " << directory << "\n";
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (fs::is_regular_file(entry.path())) {
            std::string filename = entry.path().filename().string();
            if (std::regex_match(filename, regex_pattern)) {
                std::string fullpath = entry.path().string();
                if (verbose) 
                    std::cout << "In parseRGADataFile call: found matching file: " << fullpath << std::endl;

                // Open the file
                std::ifstream file(fullpath);
                if (!file.is_open()) {
                    throw std::runtime_error("Error parseRGADataFile function call:"
                    " Could not open file: " + fullpath);
                }

                // Load all the lines into a buffer
                std::string line;
                while (std::getline(file, line)) {
                    lines.push_back(line);
                }

                file.close();
                file_found = true;
                break; // Stop searching after finding the first matching file
            }
        }
    }
    
    // Throw an error if no matching file is found
    if (!file_found) {
        throw std::runtime_error("Error parseRGADataFile function call:"
        " No matching file found for pattern: " + pattern);
    }

    // Ensure the data file has a headers row
    if (lines.empty()) {
        throw std::runtime_error("Error parseRGADataFile function call: Data file is empty.");
    }

    // Find the header row
    size_t header_row_index = 0;
    for (; header_row_index < lines.size(); ++header_row_index) {
        if (lines[header_row_index].find("Time Relative (sec)") != std::string::npos) {
            break;
        }
    }

    // Throw an error if no header row is found
    if (header_row_index == lines.size()) {
        throw std::runtime_error("Error parseRGADataFile function call: No header row found in the data file.");
    }

    // Process the headers row
    std::istringstream header_stream(lines[header_row_index]);
    std::vector<std::string> headers;
    std::string header;
    std::vector<double> bins;  // May not need
    while (std::getline(header_stream, header, '\t')) {
        if (header != "Time Relative (sec)" && header != "Time Absolute (UTC)" && header != "Time Absolute (Date_Time)" && header != "Step") {
            bins.push_back(std::stod(header));
        }
        headers.push_back(header);
    }

    // Map the header to each row entry for all entire time series
    std::unordered_map<double, std::unordered_map<std::string, std::string>> all_time_series_header_map;
    for (size_t i = header_row_index + 1; i < lines.size(); ++i) {
        std::vector<std::string> current_row = FileReader::split(lines.at(i), "\t");

        // Check that the numbers of headers and the current line match
        if (current_row.size() != headers.size())
            throw std::runtime_error("Error parseRGADataFile function call: Number of headers and row entries do not match.");
        
        // Map the headers to the current row entries
        // THIS ASSUMES THAT THE UNIX TIME IS THE SECOND ENTRY IN THE ROW
        double unix_time;
        try {
            unix_time = std::stod(current_row.at(1));
        } catch (const std::exception& e) {
            if (verbose) std::cerr << "Error parseRGADataFile function call: error parsing unix time: " << e.what() << "\n";
            throw std::runtime_error("Error parseRGADataFile function call: Error parsing unix time");
        }
        for (size_t j = 0; j < headers.size(); ++j) {
            all_time_series_header_map[unix_time][headers.at(j)] = current_row.at(j);
        }
    }

    // Extract the AMUBins struct from input RGAData object
    std::vector<RGAData::AMUBins> rga_object_bins = rga_data.getBins();

    // Add time-series data into RGAData object
    for (const auto& AMUbin_object : rga_object_bins) {
        std::cout << "Current bin: "; AMUbin_object.print();
        for (const auto& time_header_map : all_time_series_header_map) {
            // Extract the time and value from the time_series and header map
            double current_input_unix_time = time_header_map.first;

            // Calculate the average of the bin values
            double current_input_value = 0.0;
            for (const auto& bin : AMUbin_object.bins) {
                // Extract the value from the time_series and header map
                std::ostringstream bin_stream;
                bin_stream.precision(2);
                bin_stream << std::fixed << bin;
                std::string bin_search = std::move(bin_stream).str();
                if (time_header_map.second.find(bin_search) == time_header_map.second.end())
                    throw std::runtime_error("Error parseRGADataFile function call: bin value not found in header map.");

                // Add the value to the current input value
                current_input_value += stod(time_header_map.second.at(bin_search));
            }
            current_input_value /= AMUbin_object.bins.size();
            
            // Add the time-series data to the RGAData object
            if (verbose) {
                std::cout << "Adding data to RGAData object: "
                << std::setprecision(9) << current_input_unix_time 
                << ", " << current_input_value 
                << "\n";
            }
            rga_data.addData(AMUbin_object, current_input_unix_time, current_input_value);
        }
    }
}