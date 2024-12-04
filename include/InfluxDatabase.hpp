#ifndef INFLUXDATABASE_HPP
#define INFLUXDATABASE_HPP

#include "influxdb.hpp"
#include "Common.hpp"

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
    bool getConnectionStatus() const { return isConnected; }
    bool checkConnection(bool verbose = false);


    // Writing to bucket
    bool writeData(const std::string& measurement, const std::string& tags,
                   const std::string& fields, long long timestamp = 0, bool verbose = false);
    
    // Querying data
    std::string queryData(const std::string& query, bool verbose = false);

    // Writing batch to bucket
    bool writeBatchData(const std::vector<std::string>& dataPoints, bool verbose = false);
    bool writeBatchData2(const std::vector<std::string>& dataPoints, bool verbose = false);

    // Parsing query
    std::vector<std::unordered_map<std::string, std::string>> parseQueryResult(const std::string& response);
    

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
};

#endif // INFLUXDATABASE_HPP