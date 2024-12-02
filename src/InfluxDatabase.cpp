#include "InfluxDatabase.hpp"

InfluxDatabase::InfluxDatabase() : isConnected(false), serverInfo("localhost", 8086, ""){}

InfluxDatabase::~InfluxDatabase() {
    disconnect();
}

bool InfluxDatabase::connect(const std::string& host, int port, const std::string& db,
                             const std::string& user, const std::string& password,
                             const std::string& precision, bool verbose) {
    serverInfo = influxdb_cpp::server_info(host, port, db, user, password, precision);
    
    // Test connection by sending a simple query or ping
    std::string response;
    int result = influxdb_cpp::query(response, "SHOW DATABASES", serverInfo);
    if (result != 0) {
        if (verbose) {
            std::cerr << "Failed to connect to InfluxDB: " << response << std::endl;
        }
        throw std::runtime_error("Failed to connect to InfluxDB: " + response);
    }

    isConnected = true;
    if (verbose) {
        std::cout << "Connected to InfluxDB at " << host << ":" << port << std::endl;
    }
    return true;
}

void InfluxDatabase::disconnect(bool verbose) {
    if (isConnected) {
        if (verbose) {
            std::cout << "Disconnecting from InfluxDB..." << std::endl;
        }
        isConnected = false; // Reset connection status
    }
}

bool InfluxDatabase::checkConnection(bool verbose) {
    if (!isConnected) {
        if (verbose) {
            std::cerr << "Not connected to InfluxDB." << std::endl;
        }
        return false;
    }
    std::string response;
    int result = influxdb_cpp::query(response, "SHOW DATABASES", serverInfo);
    if (result != 0) {
        if (verbose) {
            std::cerr << "Connection test failed: " << response << std::endl;
        }
        isConnected = false;
        return false;
    }
    if (verbose) {
        std::cout << "Connection is healthy." << std::endl;
    }
    return true;
}

bool InfluxDatabase::writeData(const std::string& measurement, const std::string& tags,
                               const std::string& fields, long long timestamp, bool verbose) {
    if (!isConnected) {
        if (verbose) {
            std::cerr << "Error: Not connected to InfluxDB." << std::endl;
        }
        throw std::runtime_error("Cannot write data: Not connected to InfluxDB.");
    }

    // Construct line protocol manually
    std::ostringstream lineProtocol;
    lineProtocol << measurement;
    if (!tags.empty()) {
        lineProtocol << "," << tags;
    }
    lineProtocol << " " << fields;
    if (timestamp > 0) {
        lineProtocol << " " << timestamp;
    }

    // Send data
    std::string response;
    int result = influxdb_cpp::detail::inner::http_request("POST", "write", "", lineProtocol.str(), serverInfo, &response);
    if (result != 0) {
        if (verbose) {
            std::cerr << "Error writing data to InfluxDB: " << response << std::endl;
        }
        throw std::runtime_error("Failed to write data to InfluxDB: " + response);
    }

    if (verbose) {
        std::cout << "Data written successfully: " << lineProtocol.str() << std::endl;
    }
    return true;
}

bool InfluxDatabase::writeBatchData(const std::vector<std::string>& dataPoints, bool verbose) {
    if (!isConnected) {
        throw std::runtime_error("Cannot write data: Not connected to InfluxDB.");
    }

    std::ostringstream batchStream;
    for (const auto& point : dataPoints) {
        batchStream << point << "\n";
    }

    std::string response;
    int result = influxdb_cpp::detail::inner::http_request("POST", "write", "", batchStream.str(), serverInfo, &response);
    if (result != 0) {
        if (verbose) {
            std::cerr << "Error writing batch data: " << response << std::endl;
        }
        throw std::runtime_error("Failed to write batch data to InfluxDB: " + response);
    }

    if (verbose) {
        std::cout << "Batch data written successfully." << std::endl;
    }
    return true;
}

std::string InfluxDatabase::queryData(const std::string& query, bool verbose) {
    if (query.empty()) {
        if (verbose) {
            std::cerr << "Error: Query string is empty." << std::endl;
        }
        throw std::invalid_argument("Query string is empty.");
    }

    std::string response;
    int result = influxdb_cpp::query(response, query, serverInfo);
    if (result != 0) {
        if (verbose) {
            std::cerr << "Error querying InfluxDB: " << response << std::endl;
        }
        throw std::runtime_error("Failed to query InfluxDB: " + response);
    }

    if (verbose) {
        std::cout << "Query executed successfully. Response: " << response << std::endl;
    }

    return response;
}

std::vector<std::unordered_map<std::string, std::string>> InfluxDatabase::parseQueryResult(const std::string& response) {
    std::vector<std::unordered_map<std::string, std::string>> parsedData;

    if (response.empty()) {
        return parsedData; // Return an empty vector for an empty response
    }

    std::istringstream responseStream(response);
    std::string line;

    // Parse headers (first line)
    std::vector<std::string> headers;
    if (std::getline(responseStream, line)) {
        std::istringstream headerStream(line);
        std::string header;
        while (std::getline(headerStream, header, ',')) {
            headers.push_back(header);
        }
    }

    // Parse rows
    while (std::getline(responseStream, line)) {
        std::istringstream rowStream(line);
        std::string value;
        std::unordered_map<std::string, std::string> row;
        size_t colIndex = 0;

        while (std::getline(rowStream, value, ',')) {
            if (colIndex < headers.size()) {
                row[headers[colIndex]] = value;
            }
            ++colIndex;
        }

        if (!row.empty()) {
            parsedData.push_back(row);
        }
    }

    return parsedData;
}
