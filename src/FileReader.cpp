#include "FileReader.hpp"

bool FileReader::loadStockDataFromFile(const std::string& filename, Stock& stock) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << filename << "\n";
        return false;
    }

    std::string line;
    int lineNumber = 0; // Keep track of the line number for error messages

    while (std::getline(file, line)) {
        lineNumber++;
        std::istringstream iss(line);
        std::string date, adjClose, close, high, low, open, volume;

        // Read each expected field from the line
        if (std::getline(iss, date, ',') && 
            std::getline(iss, open, ',') &&
            std::getline(iss, high, ',') &&
            std::getline(iss, low, ',') &&
            std::getline(iss, close, ',') &&
            std::getline(iss, adjClose, ',') &&
            std::getline(iss, volume)) {
            
			// Trim date string
			date = trimInternal(date);
            try {
                // Convert and add data only if conversion is successful
                stock.addData(date, "adj close", std::stod(adjClose));
                stock.addData(date, "close", std::stod(close));
                stock.addData(date, "high", std::stod(high));
                stock.addData(date, "low", std::stod(low));
                stock.addData(date, "open", std::stod(open));
                stock.addData(date, "volume", std::stod(volume));
            } catch (const std::invalid_argument& e) {
                std::cerr << "Warning FileReader::loadStockDataFromFile: Invalid data format at line " << lineNumber << ": " << line << "\n";
                continue; // Skip this line and move to the next
            } catch (const std::out_of_range& e) {
                std::cerr << "Warning FileReader::loadStockDataFromFile: Number out of range at line " << lineNumber << ": " << line << "\n";
                continue; // Skip this line and move to the next
            }
        } else {
            std::cerr << "Warning FileReader::loadStockDataFromFile: Malformed line at " << lineNumber << ": " << line << "\n";
            continue; // Skip this line and move to the next
        }
    }

    file.close();
    return true;
}

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