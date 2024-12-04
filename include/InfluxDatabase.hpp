#ifndef INFLUXDATABASE_HPP
#define INFLUXDATABASE_HPP

#include "influxdb.hpp"
#include "Common.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

// For outputting the result of the write operation
struct WriteResult {
    bool success;
    std::string error_message;
    std::string data_out;
};

// For outputting the result of the query operation
struct DataPoint {
    std::unordered_map<std::string, std::string> tags;
    std::unordered_map<std::string, std::string> fields;
    std::string time;
    std::string measurement;
};

class InfluxDatabase {
public:
    // Constructors and destructors
    InfluxDatabase(const std::string& host, int port, 
                    const std::string& org, const std::string& db, 
                    const std::string& user, const std::string& password, 
                    const std::string& precision, const std::string& token,
                    bool verbose = false);
    InfluxDatabase();
    ~InfluxDatabase();

    // Connection and disconnections
    bool connect(const std::string& host, int port, 
                const std::string& org, const std::string& db,
                const std::string& user, const std::string& password,
                const std::string& precision, const std::string& token,
                bool verbose);
    void disconnect(bool verbose = false);
    bool getConnectionStatus() const { return isConnected; }
    bool checkConnection(bool verbose = false);


    // Writing to bucket (not working)
    bool writeData(const std::string& measurement, const std::string& tags,
                   const std::string& fields, long long timestamp = 0, bool verbose = false);
    
    // Querying data
    std::string queryData(const std::string& query, bool verbose = false);
    std::string queryData2(const std::string& query, bool verbose);

    // Writing batch to bucket (not working)
    bool writeBatchData(const std::vector<std::string>& dataPoints, bool verbose = false);
    
    // Parsing query 
    std::vector<std::unordered_map<std::string, std::string>> parseQueryResult(const std::string& response);
    std::vector<DataPoint> parseQueryResponse(const std::string& response);

    // Writing bulk data
    WriteResult writeBulkData(const std::vector<std::string>& data_points, bool verbose = false);

private:
    std::string host_;
    int port_;
    std::string org_;
    std::string db_;
    std::string user_;
    std::string password_;
    std::string precision_;
    std::string token_;
    influxdb_cpp::server_info serverInfo;
    bool isConnected;



    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif // INFLUXDATABASE_HPP