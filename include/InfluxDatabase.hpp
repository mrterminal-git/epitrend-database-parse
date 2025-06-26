#ifndef INFLUXDATABASE_HPP
#define INFLUXDATABASE_HPP

#include "Common.hpp"
#include "influxdb.hpp"
#include "EpitrendBinaryData.hpp"
#include "RGAData.hpp"

#include <curl/curl.h>

/**
 * @class CurlHeaders
 * @brief A helper class to manage cURL headers and prevent memory allocation issues.
 *
 * The `CurlHeaders` class provides functionality to append headers and manage their lifecycle
 * to avoid memory leaks when using cURL for HTTP requests.
 */
class CurlHeaders {
public:
    /**
     * @brief Default constructor for CurlHeaders.
     */
    CurlHeaders() : headers_(nullptr) {}

    /**
     * @brief Destructor for CurlHeaders. Frees allocated memory for headers.
     */
    ~CurlHeaders() { if (headers_) curl_slist_free_all(headers_); }
    
    /**
     * @brief Appends a header to the cURL header list.
     * @param header The header string to append.
     * @throws std::runtime_error If the header cannot be appended.
     */
    void append(const std::string& header);

    /**
     * @brief Retrieves the cURL header list.
     * @return A pointer to the cURL header list.
     */
    struct curl_slist* get() const { return headers_; }

private:
    struct curl_slist* headers_; ///< Pointer to the cURL header list.
};

/**
 * @class InfluxDatabase
 * @brief A class for managing connections and interactions with an InfluxDB database.
 *
 * The `InfluxDatabase` class provides functionality to connect to an InfluxDB instance,
 * write data, query data, and parse query responses. It supports batch data writing and
 * copying data from Epitrend and RGA systems into InfluxDB buckets.
 */
class InfluxDatabase {
public:
    // Constructors and destructors
    /**
     * @brief Default constructor for InfluxDatabase.
     *
     * Initializes the database connection with default values (localhost, port 8086).
     */
    InfluxDatabase();

    /**
     * @brief Parameterized constructor for InfluxDatabase.
     * @param host The host address of the InfluxDB server.
     * @param port The port number of the InfluxDB server.
     * @param org The organization name in InfluxDB.
     * @param bucket The bucket name in InfluxDB.
     * @param user The username for authentication (optional).
     * @param password The password for authentication (optional).
     * @param precision The timestamp precision (default: "ms").
     * @param token The authentication token (optional).
     * @param verbose If true, prints connection details to the console.
     */
    InfluxDatabase(const std::string& host, int port, 
                   const std::string& org, const std::string& bucket, 
                   const std::string& user = "", const std::string& password = "", 
                   const std::string& precision = "ms", const std::string& token = "",
                   bool verbose = false);

    /**
     * @brief Destructor for InfluxDatabase.
     *
     * Disconnects from the InfluxDB server and cleans up resources.
     */
    ~InfluxDatabase();


    // Connection and disconnections
    /**
     * @brief Connects to the InfluxDB server.
     * @param host The host address of the InfluxDB server.
     * @param port The port number of the InfluxDB server.
     * @param org The organization name in InfluxDB.
     * @param bucket The bucket name in InfluxDB.
     * @param user The username for authentication (optional).
     * @param password The password for authentication (optional).
     * @param precision The timestamp precision (default: "ms").
     * @param token The authentication token (optional).
     * @param verbose If true, prints connection details to the console.
     * @return True if the connection is successful, false otherwise.
     * @throws std::runtime_error If the connection fails.
     */
    bool connect(const std::string& host, int port,
                const std::string& org, const std::string& bucket, 
                const std::string& user = "", const std::string& password = "",
                const std::string& precision = "ms", const std::string& token = "",
                bool verbose = false);

    /**
     * @brief Disconnects from the InfluxDB server.
     * @param verbose If true, prints disconnection details to the console.
     */
    void disconnect(bool verbose = false);
    
    /**
     * @brief Checks the connection status with the InfluxDB server.
     * @param verbose If true, prints connection status details to the console.
     * @return True if the connection is healthy, false otherwise.
     */
    bool checkConnection(bool verbose = false);

    /**
     * @brief Writes a single data point to the InfluxDB bucket.
     * @param measurement The measurement name.
     * @param tags The tags associated with the data point.
     * @param fields The fields associated with the data point.
     * @param timestamp The timestamp of the data point (optional).
     * @param verbose If true, prints write details to the console.
     * @return True if the data is written successfully, false otherwise.
     * @throws std::runtime_error If the write operation fails.
     */
    bool writeData(const std::string& measurement, const std::string& tags,
                   const std::string& fields, long long timestamp = 0, bool verbose = false);
    
    /**
     * @brief Queries data from the InfluxDB bucket.
     * @param query The query string in Flux language.
     * @param verbose If true, prints query details to the console.
     * @return The query response as a string.
     * @throws std::runtime_error If the query operation fails.
     */
    std::string queryData(const std::string& query, bool verbose = false);

    /**
     * @brief Queries data from the InfluxDB bucket using cURL.
     * @param response A reference to store the query response.
     * @param query The query string in Flux language.
     * @return True if the query is successful, false otherwise.
     * @throws std::runtime_error If the query operation fails.
     */
    bool queryData2(std::string& response, const std::string& query);

    /**
     * @brief Writes batch data points to the InfluxDB bucket.
     * @param dataPoints A vector of data points in line protocol format.
     * @param verbose If true, prints batch write details to the console.
     * @return True if the batch data is written successfully, false otherwise.
     * @throws std::runtime_error If the batch write operation fails.
     */
    bool writeBatchData(const std::vector<std::string>& dataPoints, bool verbose = false);

    /**
     * @brief Writes batch data points to the InfluxDB bucket using cURL.
     * @param dataPoints A vector of data points in line protocol format.
     * @param verbose If true, prints batch write details to the console.
     * @return True if the batch data is written successfully, false otherwise.
     * @throws std::runtime_error If the batch write operation fails.
     */
    bool writeBatchData2(const std::vector<std::string>& dataPoints, bool verbose = false);

    /**
     * @brief Parses the query response into a vector of key-value pairs.
     * @param response The query response string.
     * @return A vector of unordered maps representing parsed data rows.
     */
    std::vector<std::unordered_map<std::string, std::string>> parseQueryResult(const std::string& response);

    /**
     * @brief Parses the query response into a vector of key-value pairs with verbose output.
     * @param response The query response string.
     * @param verbose If true, prints parsing details to the console.
     * @return A vector of unordered maps representing parsed data rows.
     * @throws std::runtime_error If parsing fails due to mismatched headers and entries.
     */
    std::vector<std::unordered_map<std::string,std::string>> parseQueryResponse(std::string& response, bool verbose = false);

    /**
     * @brief Copies Epitrend binary data to the InfluxDB bucket.
     * @param data The EpitrendBinaryData object containing time-series data.
     * @param verbose If true, prints copy details to the console.
     * @return True if the data is copied successfully, false otherwise.
     * @throws std::runtime_error If the copy operation fails.
     */
    bool copyEpitrendToBucket(EpitrendBinaryData data, bool verbose = false);

    /**
     * @brief Copies Epitrend binary data to the InfluxDB bucket with retry logic.
     * @param data The EpitrendBinaryData object containing time-series data.
     * @param verbose If true, prints copy details to the console.
     * @return True if the data is copied successfully, false otherwise.
     * @throws std::runtime_error If the copy operation fails.
     */
    bool copyEpitrendToBucket2(EpitrendBinaryData data, bool verbose = false);

    /**
     * @brief Copies RGA data to the InfluxDB bucket.
     * @param data The RGAData object containing time-series data.
     * @param verbose If true, prints copy details to the console.
     * @return True if the data is copied successfully, false otherwise.
     * @throws std::runtime_error If the copy operation fails.
     */
    bool copyRGADataToBucket(RGAData data, bool verbose = false);


private:
    influxdb_cpp::server_info serverInfo; ///< Server information for InfluxDB connection.
    std::string host_; ///< Host address of the InfluxDB server.
    int port_; ///< Port number of the InfluxDB server.
    std::string org_; ///< Organization name in InfluxDB.
    std::string bucket_; ///< Bucket name in InfluxDB.
    std::string user_; ///< Username for authentication.
    std::string password_; ///< Password for authentication.
    std::string precision_; ///< Timestamp precision (e.g., "ms", "s").
    std::string token_; ///< Authentication token.
    bool isConnected; ///< Connection status.

    /**
     * @brief Callback function for cURL to handle HTTP responses.
     * @param contents Pointer to the response data.
     * @param size Size of each data element.
     * @param nmemb Number of data elements.
     * @param s Pointer to the string to store the response.
     * @return The size of the response data.
     */
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);

    /**
     * @brief Splits a string by a delimiter.
     * @param s The input string.
     * @param delimiter The delimiter string.
     * @return A vector of tokens.
     */
    static std::vector<std::string> split(std::string s, const std::string& delimiter);

    /**
     * @brief Trims whitespace from the end of a string.
     * @param str The input string.
     * @return The trimmed string.
     */
    static std::string trimInternal(const std::string& str);
    
    /**
     * @brief Escapes special characters for InfluxDB line protocol.
     * @param str The input string.
     * @return The escaped string.
     */
    static std::string escapeSpecialChars(const std::string& str);

    /**
     * @brief Converts days from a custom epoch to Unix epoch with the specified precision.
     * @param days The number of days from the custom epoch.
     * @return The timestamp in the specified precision.
     */
    long long convertDaysFromEpochToPrecisionFromUnix(double days);

    /**
     * @brief Converts seconds from Unix epoch to the specified precision.
     * @param unix_time_seconds The number of seconds from Unix epoch.
     * @return The timestamp in the specified precision.
     */
    long long convertSecondsFromUnixToPrecisionFromUnix(double unix_time_seconds);
};

#endif // INFLUXDATABASE_HPP