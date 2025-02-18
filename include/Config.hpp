#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

class Config {
public:
    Config(const std::string& configFilePath);

    std::string getDataDir() const;
    std::string getOutputDir() const;
    std::string getServerEpitrendDataDir() const;
    std::string getServerRGADataDir() const;
    std::string getOrg() const;
    std::string getHost() const;
    int getPort() const;
    std::string getRgaBucket() const;
    std::string getEpitrendBucket() const;
    std::string getUser() const;
    std::string getPassword() const;
    std::string getPrecision() const;
    std::string getToken() const;

private:
    void loadConfig(const std::string& configFilePath);

    std::unordered_map<std::string, std::string> configMap;
};

#endif // CONFIG_HPP