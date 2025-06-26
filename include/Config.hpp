#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

/**
 * @class Config
 * @brief A class for loading and managing configuration settings from a file.
 *
 * The `Config` class reads key-value pairs from a configuration file and provides
 * methods to retrieve specific configuration values. It ensures that non-visible
 * characters are handled appropriately and throws exceptions for missing or invalid
 * configuration files.
 */
class Config {
public:
    /**
     * @brief Constructs a Config object and loads the configuration file.
     * @param configFilePath The path to the configuration file.
     * @throws std::runtime_error If the configuration file cannot be opened.
     */
    Config(const std::string& configFilePath);

    /**
     * @brief Retrieves the directory for data files.
     * @return The value of the "DATA_DIR" key from the configuration file.
     */
    std::string getDataDir() const;

    /**
     * @brief Retrieves the directory for output files.
     * @return The value of the "OUTPUT_DIR" key from the configuration file.
     */
    std::string getOutputDir() const;

    /**
     * @brief Retrieves the directory for Epitrend data files on the server.
     * @return The value of the "SERVER_EPITREND_DATA_DIR" key from the configuration file.
     */
    std::string getServerEpitrendDataDir() const;

    /**
     * @brief Retrieves the directory for RGA data files on the server.
     * @return The value of the "SERVER_RGA_DATA_DIR" key from the configuration file.
     */
    std::string getServerRGADataDir() const;

    /**
     * @brief Retrieves the organization name for InfluxDB.
     * @return The value of the "ORG" key from the configuration file.
     */
    std::string getOrg() const;

    /**
     * @brief Retrieves the host address for InfluxDB.
     * @return The value of the "HOST" key from the configuration file.
     */
    std::string getHost() const;

    /**
     * @brief Retrieves the port number for InfluxDB.
     * @return The value of the "PORT" key from the configuration file as an integer.
     */
    int getPort() const;

    /**
     * @brief Retrieves the bucket name for RGA data in InfluxDB.
     * @return The value of the "RGA_BUCKET" key from the configuration file.
     */
    std::string getRgaBucket() const;

    /**
     * @brief Retrieves the bucket name for Epitrend data in InfluxDB.
     * @return The value of the "EPITREND_BUCKET" key from the configuration file.
     */
    std::string getEpitrendBucket() const;

    /**
     * @brief Retrieves the username for InfluxDB authentication.
     * @return The value of the "USER" key from the configuration file.
     */
    std::string getUser() const;

    /**
     * @brief Retrieves the password for InfluxDB authentication.
     * @return The value of the "PASSWORD" key from the configuration file.
     */
    std::string getPassword() const;

    /**
     * @brief Retrieves the precision for timestamps in InfluxDB.
     * @return The value of the "PRECISION" key from the configuration file.
     */
    std::string getPrecision() const;

    /**
     * @brief Retrieves the authentication token for InfluxDB.
     * @return The value of the "TOKEN" key from the configuration file.
     */
    std::string getToken() const;

private:
    /**
     * @brief Loads the configuration file and populates the configuration map.
     * @param configFilePath The path to the configuration file.
     * @throws std::runtime_error If the configuration file cannot be opened.
     */
    void loadConfig(const std::string& configFilePath);

    /**
     * @brief A map to store key-value pairs from the configuration file.
     */
    std::unordered_map<std::string, std::string> configMap;
};

#endif // CONFIG_HPP