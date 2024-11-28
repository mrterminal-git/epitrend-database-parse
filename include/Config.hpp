#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

class Config {
public:
    static const std::string DATA_DIR;
    static const std::string NYSE_LISTINGS_FILE;
    static const std::string STOCK_DATA_DIR;
	static const std::string OUTPUT_DIR;

    static std::string getListingsFilePath() {
        return DATA_DIR + NYSE_LISTINGS_FILE;
    }

    static std::string getStockDataDir() {
        return DATA_DIR + STOCK_DATA_DIR;
    }
	
	static std::string getOutputDir() {
        return OUTPUT_DIR;
    }
};

#endif // CONFIG_HPP