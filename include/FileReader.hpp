#ifndef FILEREADER_HPP
#define FILEREADER_HPP

#include <sstream>
#include <regex>
#include "Common.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"
#include "RGAData.hpp"

/**
 * @class FileReader
 * @brief A utility class for parsing time-series data files from Epitrend and RGA systems.
 *
 * The `FileReader` class provides static methods to parse binary and text files
 * containing time-series data. It supports parsing files from local directories
 * and server directories, extracting metadata, and loading data into structured objects.
 */
class FileReader {
public:
    /**
     * @brief Trims whitespace from the beginning and end of a string.
     * @param str The input string to trim.
     * @return A trimmed string with leading and trailing whitespace removed.
     */
    static std::string trim(const std::string& str);

    /**
     * @brief Parses an Epitrend binary format file and extracts metadata.
     * @param config The configuration object containing file paths.
     * @param GM The general metadata identifier (e.g., "Cluster").
     * @param year The year of the data file.
     * @param month The month of the data file (1-12).
     * @param day The day of the data file.
     * @param hour The hour of the data file.
     * @param verbose If true, prints detailed parsing information to the console.
     * @return An `EpitrendBinaryFormat` object containing parsed metadata.
     * @throws std::runtime_error If the file cannot be opened or parsed.
     */
    static EpitrendBinaryFormat parseEpitrendBinaryFormatFile(
        const Config& config,
        const std::string& GM,
        int year,
        int month,
        int day,
        int hour,
        bool verbose
    );
   
    /**
     * @brief Parses an Epitrend binary data file and loads time-series data.
     * @param config The configuration object containing file paths.
     * @param binary_data The `EpitrendBinaryData` object to store parsed data.
     * @param GM The general metadata identifier (e.g., "Cluster").
     * @param year The year of the data file.
     * @param month The month of the data file (1-12).
     * @param day The day of the data file.
     * @param hour The hour of the data file.
     * @param verbose If true, prints detailed parsing information to the console.
     * @throws std::runtime_error If the file cannot be opened or parsed.
     */
    static void parseEpitrendBinaryDataFile(
        const Config& config,
        EpitrendBinaryData& binary_data,
        const std::string& GM,
        int year,
        int month,
        int day,
        int hour,
        bool verbose
    );

    /**
     * @brief Parses an Epitrend binary format file from the server and extracts metadata.
     * @param config The configuration object containing server file paths.
     * @param GM The general metadata identifier (e.g., "Cluster").
     * @param year The year of the data file.
     * @param month The month of the data file (1-12).
     * @param day The day of the data file.
     * @param hour The hour of the data file.
     * @param verbose If true, prints detailed parsing information to the console.
     * @return An `EpitrendBinaryFormat` object containing parsed metadata.
     * @throws std::runtime_error If the file cannot be opened or parsed.
     */
    static EpitrendBinaryFormat parseServerEpitrendBinaryFormatFile(
        const Config& config,
        const std::string& GM,
        int year,
        int month,
        int day,
        int hour,
        bool verbose
    );
   
    /**
     * @brief Parses an Epitrend binary data file from the server and loads time-series data.
     * @param config The configuration object containing server file paths.
     * @param binary_data The `EpitrendBinaryData` object to store parsed data.
     * @param GM The general metadata identifier (e.g., "Cluster").
     * @param year The year of the data file.
     * @param month The month of the data file (1-12).
     * @param day The day of the data file.
     * @param hour The hour of the data file.
     * @param verbose If true, prints detailed parsing information to the console.
     * @throws std::runtime_error If the file cannot be opened or parsed.
     */
    static void parseServerEpitrendBinaryDataFile(
        const Config& config,
        EpitrendBinaryData& binary_data,
        const std::string& GM,
        int year,
        int month,
        int day,
        int hour,
        bool verbose
    );

    /**
     * @brief Parses an RGA data file and loads time-series data.
     * @param rga_data The `RGAData` object to store parsed data.
     * @param GM The general metadata identifier (e.g., "Cluster").
     * @param year The year of the data file.
     * @param month The month of the data file (1-12).
     * @param day The day of the data file.
     * @param verbose If true, prints detailed parsing information to the console.
     * @throws std::runtime_error If the file cannot be opened or parsed.
     */
    static void parseRGADataFile(
        RGAData& rga_data,
        const std::string& GM,
        int year,
        int month,
        int day,
        bool verbose
    );

    /**
     * @brief Parses an RGA data file from the server and loads time-series data.
     * @param config The configuration object containing server file paths.
     * @param rga_data The `RGAData` object to store parsed data.
     * @param GM The general metadata identifier (e.g., "Cluster").
     * @param year The year of the data file.
     * @param month The month of the data file (1-12).
     * @param day The day of the data file.
     * @param verbose If true, prints detailed parsing information to the console.
     * @throws std::runtime_error If the file cannot be opened or parsed.
     */
    static void parseServerRGADataFile(
        const Config& config,
        RGAData& rga_data,
        const std::string& GM,
        int year,
        int month,
        int day,
        bool verbose
    );

private:
	/**
     * @brief Trims whitespace from the beginning and end of a string (internal use).
     * @param str The input string to trim.
     * @return A trimmed string with leading and trailing whitespace removed.
     */
	static std::string trimInternal(const std::string& str);

    /**
     * @brief Splits a string by a delimiter (internal use).
     * @param s The input string to split.
     * @param delimiter The delimiter string.
     * @return A vector of tokens extracted from the input string.
     */
    static std::vector<std::string> split(std::string s, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t pos = 0;
        std::string token;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            tokens.push_back(token);
            s.erase(0, pos + delimiter.length());
        }
        tokens.push_back(s);

        return tokens;
    }

};

#endif // FILEREADER_H