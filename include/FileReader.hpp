#ifndef FILEREADER_HPP
#define FILEREADER_HPP

#include <sstream>
#include "Common.hpp"
#include "EpitrendBinaryFormat.hpp"

class FileReader {
public:
    // Public static method if you want to access it from other classes
    static std::string trim(const std::string& str);

    // Test function
    static EpitrendBinaryFormat parseEpitrendFile(
        int year, 
        int month, 
        int day, 
        int hour, 
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