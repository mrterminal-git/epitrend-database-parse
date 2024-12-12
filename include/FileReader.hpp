#ifndef FILEREADER_HPP
#define FILEREADER_HPP

#include <sstream>
#include <regex>
#include "Common.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"

class FileReader {
public:
    // Public static method if you want to access it from other classes
    static std::string trim(const std::string& str);

    // Parse the Epitrend binary format file
    static EpitrendBinaryFormat parseEpitrendBinaryFormatFile(
        std::string GM,
        int year, 
        int month, 
        int day, 
        int hour, 
        bool verbose
    );
   
    // Parse the Epitrend binary data file
    static void parseEpitrendBinaryDataFile(
        EpitrendBinaryData& binary_data,
        std::string GM,
        int year,
        int month,
        int day,
        int hour,
        bool verbose
    );

    // Parse the server Epitrend binary format file
    static EpitrendBinaryFormat parseServerEpitrendBinaryFormatFile(
        std::string GM,
        int year, 
        int month, 
        int day, 
        int hour, 
        bool verbose
    );
   
    // Parse the server Epitrend binary data file
    static void parseServerEpitrendBinaryDataFile(
        EpitrendBinaryData& binary_data,
        std::string GM,
        int year,
        int month,
        int day,
        int hour,
        bool verbose
    );

    // Parse the RGA data file
    static void parseRGADataFile(
        std::string GM,
        int year,
        int month,
        int day,
        bool verbose
    );

private:
	// Internal use of trimming
	static std::string trimInternal(const std::string& str);

    // Internal using of splitting by delimiter
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