#include "InfluxDatabase.hpp"

InfluxDatabase::InfluxDatabase() : isConnected(false), serverInfo("localhost", 8086, ""){}

InfluxDatabase::InfluxDatabase(const std::string& host, int port, 
                   const std::string& org, const std::string& bucket, 
                   const std::string& user, const std::string& password, 
                   const std::string& precision, const std::string& token,
                   bool verbose)
     : isConnected(false), serverInfo("localhost", 8086, ""){
        connect(host, port, org, bucket, user, password, precision, token, verbose);
     }


InfluxDatabase::~InfluxDatabase() {
    disconnect();
}

bool InfluxDatabase::connect(const std::string& host, int port, 
                            const std::string& org, const std::string& bucket,
                            const std::string& user, const std::string& password,
                            const std::string& precision, const std::string& token,
                            bool verbose) {
    serverInfo = influxdb_cpp::server_info(host, port, bucket, user, password, precision, token);
    
    // Test connection by sending a simple query or ping
    std::string response;
    int result = influxdb_cpp::query(response, "SHOW DATABASES", serverInfo);
    if (result != 0) {
        if (verbose) {
            std::cerr << "Failed to connect to InfluxDB: " << response << "\n";
        }
        throw std::runtime_error("Failed to connect to InfluxDB: " + response);
    }

    host_ = host;
    port_ = port;
    org_ = org;
    bucket_ = bucket;
    user_ = user;
    password_ = password;
    precision_ = precision;
    token_ = token;
    isConnected = true;
    if (verbose) {
        std::cout << "Connected to InfluxDB at " << host << ":" << port << "\n";
    }
    return true;
}

void InfluxDatabase::disconnect(bool verbose) {
    if (isConnected) {
        if (verbose) {
            std::cout << "Disconnecting from InfluxDB..." << "\n";
        }
        serverInfo = influxdb_cpp::server_info("localhost", 8086, ""); // Reset server info
        isConnected = false; // Reset connection status
    }
}

bool InfluxDatabase::checkConnection(bool verbose) {
    if (!isConnected) {
        if (verbose) {
            std::cerr << "Not connected to InfluxDB." << "\n";
        }
        return false;
    }
    std::string response;
    int result = influxdb_cpp::query(response, "SHOW DATABASES", serverInfo);
    if (result != 0) {
        if (verbose) {
            std::cerr << "Connection test failed: " << response << "\n";
        }
        isConnected = false;
        return false;
    }
    if (verbose) {
        std::cout << "Connection is healthy." << "\n";
    }
    return true;
}

bool InfluxDatabase::writeData(const std::string& measurement, const std::string& tags,
                               const std::string& fields, long long timestamp, bool verbose) {
    if (!isConnected) {
        if (verbose) {
            std::cerr << "Error: Not connected to InfluxDB." << "\n";
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
            std::cerr << "Error writing data to InfluxDB: " << response << "\n";
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
            std::cerr << "Error writing batch data: " << response << "\n";
        }
        throw std::runtime_error("Failed to write batch data to InfluxDB: " + response);
    }

    if (verbose) {
        std::cout << "Batch data written successfully." << "\n";
    }
    return true;
}

bool InfluxDatabase::writeBatchData2(const std::vector<std::string>& dataPoints, bool verbose) {
    if (!isConnected) {
        throw std::runtime_error("Cannot write data: Not connected to InfluxDB.");
    }

    // Construct the line protocol string
    std::ostringstream batchStream;
    for (const auto& point : dataPoints) {
        batchStream << point << "\n";
    }

    std::string lineProtocol = batchStream.str();

    // Send the line protocol string to InfluxDB
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/api/v2/write?org=" + org_ + "&bucket=" + bucket_ + "&precision=" + precision_;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, lineProtocol.c_str());

        // Set the authorization header with the token
        struct curl_slist *headers = NULL;
        std::string authHeader = "Authorization: Token " + token_;
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Capture the response
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            if (verbose) {
                std::cerr << "Error in InfluxDatabase::writeBatchData2response: error writing batch data to InfluxDB\n";
            }
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            throw std::runtime_error("Failed to write batch data to InfluxDB: " + std::string(curl_easy_strerror(res)));
        }

        // Check for errors in the response
        if (response.find("\"code\":\"invalid\"") != std::string::npos) {
            if (verbose) {
                std::cerr << "Error in InfluxDatabase::writeBatchData2response: detected error from InfluxDB\n";
            }
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            throw std::runtime_error("Failed to write batch data to InfluxDB: " + response);
        }

        // Write response if batch write is successful
        if (verbose) {
            std::cout << "Batch data written successfully to org: " << org_ << ", bucket: " << bucket_ << "\n";
            std::cout << "Line protocol: \n" << lineProtocol;
            std::cout << "Response:  " + response << "\n" ;
        }

            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }

    return true;
}

size_t InfluxDatabase::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    } catch (std::bad_alloc& e) {
        // Handle memory problem
        return 0;
    }
    return newLength;
}

std::string InfluxDatabase::queryData(const std::string& query, bool verbose) {
    if (query.empty()) {
        if (verbose) {
            std::cerr << "Error: Query string is empty." << "\n";
        }
        throw std::invalid_argument("Query string is empty.");
    }

    std::string response;
    int result = influxdb_cpp::query(response, query, serverInfo);
    if (result != 0) {
        if (verbose) {
            std::cerr << "Error querying InfluxDB: " << response << "\n";
        }
        throw std::runtime_error("Failed to query InfluxDB: " + response);
    }

    if (verbose) {
        std::cout << "Query executed successfully. Response: " << response << "\n";
    }

    return response;
}

bool InfluxDatabase::queryData2(std::string& response, const std::string& query) {
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/api/v2/query?org=" + org_;
        std::string auth_header = "Authorization: Token " + token_;

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/vnd.flux");
        headers = curl_slist_append(headers, auth_header.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            curl_easy_cleanup(curl);
            return -1;
        }
        curl_easy_cleanup(curl);
    }
    return true;
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

std::vector<std::unordered_map<std::string,std::string>> InfluxDatabase::parseQueryResponse(std::string& response, bool verbose) {
    std::vector<std::unordered_map<std::string,std::string>> out;
    
    std::istringstream responseStream(response);
    std::string line;

    // Parse headers (first line)
    std::vector<std::string> headers;
    if (std::getline(responseStream, line)) {
        if(verbose) std::cout << "Line :" << line << "\n";
        headers = split(trimInternal(line), ",");
    }
    if(verbose) for(auto element : headers) {std::cout << "header|" << element << "|\n"; }

    // Parse rows
    std::vector<std::string> entries;
    while (std::getline(responseStream, line)) {
        if(trimInternal(line) == "") continue;
        std::istringstream rowStream(line);
        std::string value;
        std::unordered_map<std::string, std::string> row;
        size_t colIndex = 0;

        if(verbose) std::cout << "Current line:|" << trimInternal(line) << "|\n";
        entries = split(trimInternal(line), ",");

        if(verbose) for(auto element : entries) {std::cout << "entry|" << element << "|\n";}

        if(entries.size() != headers.size()) {
            std::cerr << "Error in InfluxDatabase::parseQueryResponse call: number of headers does not match number of entries\n";
            throw std::runtime_error("Error in InfluxDatabase::parseQueryResponse call: number of headers does not match number of entries\n");
        }
        
        std::unordered_map<std::string,std::string> headers_entries;
        for(int i = 0; i < entries.size(); i++) {
            headers_entries[headers.at(i)] = entries.at(i);
        }

        out.push_back(headers_entries);
    }

    return out;

}


// Internal using of splitting by delimiter
std::vector<std::string> InfluxDatabase::split(std::string s, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

std::string InfluxDatabase::trimInternal(const std::string& str) {
        std::string trimmed = str;
        trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), trimmed.end());
        return trimmed;
}
