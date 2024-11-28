#include "FileReader.hpp"

// bool FileReader::loadStockDataFromFile(const std::string& filename, Stock& stock) {
//     std::ifstream file(filename);
//     if (!file.is_open()) {
//         std::cerr << "Could not open the file: " << filename << "\n";
//         return false;
//     }

//     std::string line;
//     int lineNumber = 0; // Keep track of the line number for error messages

//     while (std::getline(file, line)) {
//         lineNumber++;
//         std::istringstream iss(line);
//         std::string date, adjClose, close, high, low, open, volume;

//         // Read each expected field from the line
//         if (std::getline(iss, date, ',') && 
//             std::getline(iss, open, ',') &&
//             std::getline(iss, high, ',') &&
//             std::getline(iss, low, ',') &&
//             std::getline(iss, close, ',') &&
//             std::getline(iss, adjClose, ',') &&
//             std::getline(iss, volume)) {
            
// 			// Trim date string
// 			date = trimInternal(date);
//             try {
//                 // Convert and add data only if conversion is successful
//                 stock.addData(date, "adj close", std::stod(adjClose));
//                 stock.addData(date, "close", std::stod(close));
//                 stock.addData(date, "high", std::stod(high));
//                 stock.addData(date, "low", std::stod(low));
//                 stock.addData(date, "open", std::stod(open));
//                 stock.addData(date, "volume", std::stod(volume));
//             } catch (const std::invalid_argument& e) {
//                 std::cerr << "Warning FileReader::loadStockDataFromFile: Invalid data format at line " << lineNumber << ": " << line << "\n";
//                 continue; // Skip this line and move to the next
//             } catch (const std::out_of_range& e) {
//                 std::cerr << "Warning FileReader::loadStockDataFromFile: Number out of range at line " << lineNumber << ": " << line << "\n";
//                 continue; // Skip this line and move to the next
//             }
//         } else {
//             std::cerr << "Warning FileReader::loadStockDataFromFile: Malformed line at " << lineNumber << ": " << line << "\n";
//             continue; // Skip this line and move to the next
//         }
//     }

//     file.close();
//     return true;
// }

std::vector<std::string> FileReader::readNYSEListings(const std::string& filename) {
    std::vector<std::string> nyseListings;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file: " << filename << std::endl;
        return nyseListings; // Return an empty list if the file cannot be opened
    }

    std::string line;
    while (std::getline(file, line)) {
        // Assuming each line in the file contains a single stock listing (name or ticker)
        if (!line.empty()) {
            nyseListings.push_back(trimInternal(line));
        }
    }

    file.close();
    return nyseListings;
}

// Internal trim function (if you want it private)
std::string FileReader::trimInternal(const std::string& str) {
    std::string trimmed = str;
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), trimmed.end());
    return trimmed;
}

std::string FileReader::trim(const std::string& str) {
    std::string trimmed = str;
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), trimmed.end());
    return trimmed;
}

// Test function
void FileReader::testFunction(){
    // Initialise the container for the binary formatting
    EpitrendBinaryFormat binaryFormat;
    
    const std::string& DATA_DIR = Config::getDataDir();
    const std::string& YEAR_DIR = "2024/";
    const std::string& MONTH_DIR = "11-Nov/";
    const std::string& filename = "01day-00hr.txt";
    const std::string& fullpath = DATA_DIR + YEAR_DIR + MONTH_DIR + filename;

    std::cout << "Opening file: " << fullpath << "\n"; // DIAGNOSTIC

    std::ifstream file(fullpath);
    
    std::vector<std::string> all_lines;
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file: " << fullpath << std::endl;
    }

    std::string line;
    std::vector<std::string> line_tokens;
    int counter  = 1;  // DIAGNOSTIC
    while (std::getline(file, line)) {
        // Assuming each line in the file contains at least one character
        if (!line.empty()) {all_lines.push_back(trimInternal(line));}

        std::cout << "-------------------------------\nLine " << counter << ": " << line << "\n"; // DIAGNOSTIC
        counter++;
        
        // Check if the line contains the = character
        if(line.find("=") == std::string::npos){ continue;}
        
        // Get the tokens after split by "=" character
        line_tokens = split(line, "=");
    
        if (line_tokens.at(0) == "TotalDataItems") {
            std::cout << "Reading TotalDataItems" << "\n";
            try{
                binaryFormat.setTotalDataItems(stoi(line_tokens.at(1)));

            } catch(std::exception& e) {
                std::cout << "testFunction error \"" << e.what() << "\"\n";
                return;

            }
            
        } else if (line_tokens.at(0) == "Misc") {
            std::cout << "Reading Misc" << "\n";
            std::vector<std::string> misc_line_tokens = split(line_tokens.at(1), ";");
            std::vector<std::string> misc_line_tokenss = split(misc_line_tokens.at(0), ":");
            
            try{
                binaryFormat.setTimeResoltuion(stod(misc_line_tokenss.at(1)));

            } catch(std::exception& e) {
                std::cout << "testFunction error \"" << e.what() << "\"\n";
                return;

            }

        } else if (line_tokens.at(0) == "CurrentDay") {
            std::cout << "Reading CurrentDay" << "\n";

            try{
                binaryFormat.setCurrentDay(stoi(line_tokens.at(1)));

            } catch(std::exception& e) {
                std::cout << "testFunction error \"" << e.what() << "\"\n";
                return;

            }

        } else if (line_tokens.at(0) == "DataItem") {
            std::cout << "Reading DataItem" << "\n";

            // Check the line contains the delimiter ";"
            if(line.find(";") == std::string::npos){ continue;}

            // Split each DataItem by ";"
            std::vector<std::string> data_item_tokens = split(line_tokens.at(1), ";");

            // Check that there are a non-zero number of DataItems
            if(data_item_tokens.size() == 0) { continue;}

            // For each data item token, split each token by ":"
            for(auto element : data_item_tokens){
                std::cout << "Reading DataItem token: " << element << "\n";
                std::vector<std::string> data_item_tokenss = split(element, ":");

                // Check that are more than two tokens inside DataItems
                if(data_item_tokenss.size() < 2) { continue;}

                // Initialize data structure to hold each data item
                EpitrendBinaryFormat::dataItem current_data_item;

                // Check what category each token is, but ignore if value is empty
                if (data_item_tokenss.at(0) == "Name" && !data_item_tokenss.at(1).empty()) {
                    std::cout << "Reading Name: " << data_item_tokenss.at(1) << "\n";
                    current_data_item.Name = data_item_tokenss.at(1);
                    
                } else if (data_item_tokenss.at(0) == "Type" && !data_item_tokenss.at(1).empty()) {
                    std::cout << "Reading Type: " << data_item_tokenss.at(1) << "\n";
                    current_data_item.Type = data_item_tokenss.at(1);

                } else if (data_item_tokenss.at(0) == "Range" && !data_item_tokenss.at(1).empty()) {
                    std::cout << "Reading Range: " << data_item_tokenss.at(1) << "\n";
                    current_data_item.Range = data_item_tokenss.at(1);

                } else if (data_item_tokenss.at(0) == "TotalValues" && !data_item_tokenss.at(1).empty()) {
                    std::cout << "Reading TotalValues: " << data_item_tokenss.at(1) << "\n";
                    current_data_item.TotalValues = stoi(data_item_tokenss.at(1));

                } else if (data_item_tokenss.at(0) == "ValueOffsets" && !data_item_tokenss.at(1).empty()){
                    std::cout << "Reading ValueOffsets: " << data_item_tokenss.at(1) << "\n";

                }
 
            }
            
        }
        
        if(counter > 10) { return;}
    }

}