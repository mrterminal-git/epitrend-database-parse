#ifndef INFLUXDATABASE_HPP
#define INFLUXDATABASE_HPP

#include "Common.hpp"
#include "influxdb.hpp"
#include "EpitrendBinaryData.hpp"
#include "RGAData.hpp"

#include <curl/curl.h>

// Helper class to properly handle curl headers and prevent memory allocation issues
class CurlHeaders {
public:
    CurlHeaders() : headers_(nullptr) {}
    ~CurlHeaders() { if (headers_) curl_slist_free_all(headers_); }
    void append(const std::string& header);
    struct curl_slist* get() const { return headers_; }

private:
    struct curl_slist* headers_;
};

class InfluxDatabase {
public:
    // Constructors and destructors
    InfluxDatabase(const std::string& host, int port, 
                   const std::string& org, const std::string& bucket, 
                   const std::string& user = "", const std::string& password = "", 
                   const std::string& precision = "ms", const std::string& token = "",
                   bool verbose = false);
    InfluxDatabase();
    ~InfluxDatabase();

    // Connection and disconnections
    bool connect(const std::string& host, int port,
                const std::string& org, const std::string& bucket, 
                const std::string& user = "", const std::string& password = "",
                const std::string& precision = "ms", const std::string& token = "",
                bool verbose = false);
    void disconnect(bool verbose = false);
    // bool getConnectionStatus() const { return isConnected; }
    bool checkConnection(bool verbose = false);


    // Writing to bucket
    bool writeData(const std::string& measurement, const std::string& tags,
                   const std::string& fields, long long timestamp = 0, bool verbose = false);
    
    // Querying data
    std::string queryData(const std::string& query, bool verbose = false);
    bool queryData2(std::string& response, const std::string& query);

    // Writing batch to bucket
    bool writeBatchData(const std::vector<std::string>& dataPoints, bool verbose = false);
    bool writeBatchData2(const std::vector<std::string>& dataPoints, bool verbose = false);

    // Parsing query
    std::vector<std::unordered_map<std::string, std::string>> parseQueryResult(const std::string& response);
    std::vector<std::unordered_map<std::string,std::string>> parseQueryResponse(std::string& response, bool verbose = false);

    // Copying to bucket
    bool copyEpitrendToBucket(EpitrendBinaryData data, bool verbose = false);
    bool copyEpitrendToBucket2(EpitrendBinaryData data, bool verbose = false);
    bool copyRGADataToBucket(RGAData data, bool verbose = false);


private:
    influxdb_cpp::server_info serverInfo;
    std::string host_;
    int port_;
    std::string org_;
    std::string bucket_;
    std::string user_;
    std::string password_;
    std::string precision_;
    std::string token_;
    bool isConnected;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);

    // Internal using of splitting by delimiter
    static std::vector<std::string> split(std::string s, const std::string& delimiter);

    // Internal trim function 
    static std::string trimInternal(const std::string& str);
    
    // Internal function to escape special characters for influxDB
    static std::string escapeSpecialChars(const std::string& str);

    // Internal function to convert days from Epoch to precision_ from Unix Epoch
    long long convertDaysFromEpochToPrecisionFromUnix(double days);

    // Internal function to convert seconds from Unix to precision_ from Unix Epoch
    long long convertSecondsFromUnixToPrecisionFromUnix(double unix_time_seconds);
};

#endif // INFLUXDATABASE_HPP