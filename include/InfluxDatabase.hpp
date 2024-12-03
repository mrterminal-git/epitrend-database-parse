#ifndef INFLUXDATABASE_HPP
#define INFLUXDATABASE_HPP

#include "influxdb.hpp"
#include "Common.hpp"

class InfluxDatabase {
private:
    influxdb_cpp::server_info serverInfo;
    bool isConnected;

public:
    InfluxDatabase();
    ~InfluxDatabase();

    bool connect(const std::string& host, int port, const std::string& db,
                 const std::string& user = "", const std::string& password = "",
                 const std::string& precision = "ms", const std::string& token = "",
                 bool verbose = false);
    void disconnect(bool verbose = false);
    bool getConnectionStatus() const { return isConnected; }
    bool checkConnection(bool verbose = false);

    InfluxDatabase(const std::string& host, int port, const std::string& db, 
                   const std::string& user = "", const std::string& password = "", 
                   const std::string& precision = "ms");

    bool writeData(const std::string& measurement, const std::string& tags,
                   const std::string& fields, long long timestamp = 0, bool verbose = false);
    std::string queryData(const std::string& query, bool verbose = false);
    bool writeBatchData(const std::vector<std::string>& dataPoints, bool verbose = false);
    std::vector<std::unordered_map<std::string, std::string>> parseQueryResult(const std::string& response);
    
};

#endif // INFLUXDATABASE_HPP