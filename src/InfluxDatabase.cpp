#include "InfluxDatabase.hpp"

using json = nlohmann::json;

InfluxDatabase::InfluxDatabase() : isConnected(false), serverInfo("localhost", 8086, ""){}

InfluxDatabase::InfluxDatabase(const std::string& host, int port, 
                                const std::string& org, const std::string& db, 
                                const std::string& user, const std::string& password, 
                                const std::string& precision, const std::string& token,
                                bool verbose)
     : isConnected(false), serverInfo("localhost", 8086, ""){
        connect(host, port, org, db, user, password, precision, token, verbose);
     }


InfluxDatabase::~InfluxDatabase() {
    disconnect();
}

bool InfluxDatabase::connect(const std::string& host, int port, 
                             const std::string& org, const std::string& db,
                             const std::string& user, const std::string& password,
                             const std::string& precision, const std::string& token,
                             bool verbose) {
    serverInfo = influxdb_cpp::server_info(host, port, db, user, password, precision, token);
    
    // Test connection by sending a simple query or ping
    std::string response;
    int result = influxdb_cpp::query(response, "SHOW DATABASES", serverInfo);
    if (result != 0) {
        if (verbose) {
            std::cerr << "Failed to connect to InfluxDB: " << response << std::endl;
        }
        throw std::runtime_error("Failed to connect to InfluxDB: " + response);
    }

    host_ = host;
    port_ = port;
    org_ = org;
    db_ = db;
    user_ = user;
    password_ = password;
    precision_ = precision;
    token_ = token;

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
        serverInfo = influxdb_cpp::server_info("localhost", 8086, ""); // Reset server info
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
        std::cout << "Data written successfully: " << lineProtocol.str() << "\n"
        << "Reponse: " << response << "\n";
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
        }
        throw std::runtime_error("Failed to query InfluxDB: " + response);
    }

    if (verbose) {
        std::cout << "Query executed successfully. Response: " << response << std::endl;
    }

    return response;
}

std::string InfluxDatabase::queryData2(const std::string& query, bool verbose) {
    if (query.empty()) {
        if (verbose) {
            std::cerr << "Error: Query string is empty." << std::endl;
        }
        throw std::invalid_argument("Query string is empty.");
    }

    std::string response;
    std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/api/v2/query?org=" + org_;
    std::string auth_header = "Authorization: Token " + token_;

    CURL* curl = curl_easy_init();
    if(curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/vnd.flux");
        headers = curl_slist_append(headers, auth_header.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            if (verbose) {
                std::cerr << "Error querying InfluxDB: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
            throw std::runtime_error("Failed to query InfluxDB: " + std::string(curl_easy_strerror(res)));
        }

        curl_easy_cleanup(curl);
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

std::vector<DataPoint> InfluxDatabase::parseQueryResponse(const std::string& response) {
    std::vector<DataPoint> parsed_data;
    std::istringstream response_stream(response);
    std::string line;

    // Parse the header line to get the column names
    std::getline(response_stream, line);
    std::istringstream header_stream(line);
    std::vector<std::string> columns;
    std::string column;
    while (std::getline(header_stream, column, ',')) {
        columns.push_back(column);
    }

    // Parse each line of the response
    while (std::getline(response_stream, line)) {
        std::istringstream line_stream(line);
        DataPoint data_point;
        std::string value;
        for (size_t i = 0; i < columns.size(); ++i) {
            std::getline(line_stream, value, ',');
            if (columns[i] == "_time") {
                data_point.time = value;
            } else if (columns[i] == "_measurement") {
                data_point.measurement = value;
            } else if (columns[i] == "_value") {
                data_point.fields["_value"] = value;
            } else if (columns[i] == "_field") {
                data_point.fields["_field"] = value;
            } else {
                data_point.tags[columns[i]] = value;
            }
        }
        parsed_data.push_back(data_point);
    }

    return parsed_data;
}

WriteResult InfluxDatabase::writeBulkData(const std::vector<std::string>& data_points, bool verbose) {
    WriteResult result;
    result.success = true;

    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/api/v2/write?org=" + org_ + "&bucket=" + db_ + "&precision=" + precision_;
        std::string auth_header = "Authorization: Token " + token_;

        std::string bulk_data;
        for (const auto& point : data_points) {
            bulk_data += point + "\n";
        }

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: text/plain");
        headers = curl_slist_append(headers, auth_header.c_str());

        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bulk_data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            if (verbose) {
                std::cerr << "Failed to write bulk data: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
            result.success = false;
            result.error_message = curl_easy_strerror(res);
            result.data_out = bulk_data;
            return result;
        }

        // Check if the response is empty
        if (response.empty()) {
            if (verbose) {
                std::cerr << "Empty response from server." << std::endl;
            }
            curl_easy_cleanup(curl);
            result.success = true;
            result.error_message = "Empty response from server.";
            result.data_out = bulk_data;
            return result;
        }

        // Parse the JSON response to check for errors
        try {
            auto json_response = json::parse(response);
            if (json_response.contains("code") && json_response["code"] != "success") {
                if (verbose) {
                    std::cerr << "Error writing bulk data: " << json_response.dump() << std::endl;
                }
                curl_easy_cleanup(curl);
                result.success = false;
                result.error_message = json_response.dump();
                result.data_out = bulk_data;
                return result;
            }
        } catch (const json::parse_error& e) {
            if (verbose) {
                std::cerr << "Failed to parse response: " << e.what() << std::endl;
            }
            curl_easy_cleanup(curl);
            result.success = false;
            result.error_message = e.what();
            result.data_out = bulk_data;
            return result;
        }

        if (verbose) {
            std::cout << "Bulk data written successfully. Response: " << response << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    return result;
}

size_t InfluxDatabase::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}