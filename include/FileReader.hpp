#ifndef FILEREADER_HPP
#define FILEREADER_HPP

#include "Stock.hpp"
#include <sstream>

class FileReader {
public:
    // Static method to load data from a file into a Stock object
    static bool loadStockDataFromFile(const std::string& filename, Stock& stock);
	
	// Static method to load NYSE stock listing from a file
    static std::vector<std::string> readNYSEListings(const std::string& filename);

    // Public static method if you want to access it from other classes
    static std::string trim(const std::string& str);

private:
	// Private if only used internally
	static std::string trimInternal(const std::string& str);
};

#endif // FILEREADER_H