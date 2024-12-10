#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

class Config {
public:
    static const std::string DATA_DIR;
	static const std::string OUTPUT_DIR;
    static const std::string SERVER_EPITREND_DATA_DIR;

    static std::string getDataDir() {return DATA_DIR;}
	static std::string getOutputDir() {return OUTPUT_DIR;}
    static std::string getServerEpitrendDataDir() {return SERVER_EPITREND_DATA_DIR;}

};

#endif // CONFIG_HPP