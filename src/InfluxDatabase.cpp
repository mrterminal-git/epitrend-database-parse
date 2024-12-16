#include "InfluxDatabase.hpp"

void CurlHeaders::append(const std::string& header) {
    headers_ = curl_slist_append(headers_, header.c_str());
    if (!headers_) {
        throw std::runtime_error("Failed to append header.");
    }
}

//=============================END OF CURLHEADERS CLASS METHODS==============================

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
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize cURL.");
    }

    std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/api/v2/query?org=" + org_;
    CurlHeaders headers;
    headers.append("Content-Type: application/vnd.flux");
    headers.append("Authorization: Token " + token_);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("cURL query failed: " + std::string(curl_easy_strerror(res)));
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

// Internal trim function
std::string InfluxDatabase::trimInternal(const std::string& str) {
        std::string trimmed = str;
        trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), trimmed.end());
        return trimmed;
}

// Internal function to escape special characters for influxDB
std::string InfluxDatabase::escapeSpecialChars(const std::string& str) {
    std::string escaped = str;
    std::string special_chars = " ,=";
    for (char c : special_chars) {
        std::string to_replace(1, c);
        std::string replacement = "\\" + to_replace;
        size_t pos = 0;
        while ((pos = escaped.find(to_replace, pos)) != std::string::npos) {
            escaped.replace(pos, 1, replacement);
            pos += replacement.length();
        }
    }
    return escaped;
}

long long InfluxDatabase::convertDaysFromEpochToPrecisionFromUnix(double days) {
    // Convert days from 1899 epoch to Unix epoch
    const double daysFrom1899To1970 = 25569.0;
    double unixDays = days - daysFrom1899To1970;

    // Convert days to seconds
    double seconds = unixDays * 86400.0; // 86400 seconds in a day

    if (precision_ == "n") {
        return static_cast<long long>(seconds * 1000000000.0); // Convert to nanoseconds
    } else if (precision_ == "u") {
        return static_cast<long long>(seconds * 1000000.0); // Convert to microseconds
    } else if (precision_ == "ms") {
        return static_cast<long long>(seconds * 1000.0); // Convert to milliseconds
    } else if (precision_ == "s") {
        return static_cast<long long>(seconds); // Already in seconds
    } else if (precision_ == "m") {
        return static_cast<long long>(seconds / 60.0); // Convert to minutes
    } else if (precision_ == "h") {
        return static_cast<long long>(seconds / 3600.0); // Convert to hours
    } else {
        throw std::invalid_argument("Invalid precision");
    }
}

long long InfluxDatabase::convertSecondsFromUnixToPrecisionFromUnix(double unix_time_seconds) {
    if (precision_ == "n") {
        return static_cast<long long>(unix_time_seconds * 1000000000.0); // Convert to nanoseconds
    } else if (precision_ == "u") {
        return static_cast<long long>(unix_time_seconds * 1000000.0); // Convert to microseconds
    } else if (precision_ == "ms") {
        return static_cast<long long>(unix_time_seconds * 1000.0); // Convert to milliseconds
    } else if (precision_ == "s") {
        return static_cast<long long>(unix_time_seconds); // Already in seconds
    } else if (precision_ == "m") {
        return static_cast<long long>(unix_time_seconds / 60.0); // Convert to minutes
    } else if (precision_ == "h") {
        return static_cast<long long>(unix_time_seconds / 3600.0); // Convert to hours
    } else {
        throw std::invalid_argument("Invalid precision");
    }
}

bool InfluxDatabase::copyEpitrendToBucket(EpitrendBinaryData data, bool verbose){
    // Batch size
    const int batchSize = 1000;
    const std::string epitrend_machine_name = "GEN200";

    // Prepare time-series (ts) query write statement e.g.
    //measurement + ",sensor_id_=1 num=299i 1735728000000";
    struct ts_write_struct {
        std::string sensor_id;
        std::string num;  // MUST BE A DECIMAL
        std::string timestamp;
        std::string write_query;
        void set_write_query(){write_query ="ts,sensor_id_=" + 
            sensor_id + " num=" + 
            num + " " + timestamp;
        }
    };

    // Prepare name-series (ns) query write statement e.g.
    //measurement + ",machine_=\"machine.name.2\",sensor_=\"sensor.name.2\" sensor_id=\"4\" " + std::to_string(default_ns_timestamp);
    struct ns_write_struct {
        std::string machine_name;
        std::string sensor_name;
        std::string sensor_id;
        std::string default_timestamp = "2000000000000";
        std::string write_query;
        void set_write_query(){write_query = "ns,machine_=" + escapeSpecialChars(machine_name) + 
            ",sensor_=" + escapeSpecialChars(sensor_name) + 
            " sensor_id=\"" + sensor_id + 
            "\" " + default_timestamp;
        }
    };

    // Prepare name-series (ns) query read statement
    //     "from(bucket: \"test-bucket\")"
    //   "|> range(start: -100y, stop: 50y)"
    //   "|> filter(fn: (r) => r[\"_measurement\"] == \"ns\")"
    ;
    struct ns_read_struct {
        std::string bucket;
        std::string machine_name;
        std::string sensor_name;
        std::string read_query;
        void set_read_query(){read_query = "from(bucket: \"" + bucket + "\") "
            "|> range(start: -50y, stop: 100y)"
            "|> filter(fn: (r) => r[\"_measurement\"] == \"ns\")" 
            "|> filter(fn: (r) => r[\"sensor_\"] == \"" + sensor_name + "\")"
            "|> filter(fn: (r) => r[\"machine_\"] == \"" + machine_name + "\")";
        }
    };

    // Prepare name-series (ns) query read all data statement
    struct ns_read_all_struct {
        std::string bucket;
        std::string read_query;
        void set_read_query(){read_query = "from(bucket: \"" + bucket + "\") "
            "|> range(start: -50y, stop: 100y)"
            "|> filter(fn: (r) => r[\"_measurement\"] == \"ns\")";
        }
    };

    // Loop through all data
    std::unordered_map<std::string, std::unordered_map<double,double>> raw_data = data.getAllTimeSeriesData();
    for(const auto& name_data_map : raw_data) {
        // CHECK IF PART NAME IS IN NS TABLE
        // IF IT ISN'T
            // ADD ENTRY OF MACHINE NAME AND PART NAME INTO TABLE
            // GET NEW SENSOR_ID FOR NAME AND ALSO INTO TABLE
        // IF IT IS
            // GET THE SENSOR_ID
        // ENTER ALL ASSOCIATED DATA INTO TS TABLE WITH ASSOCIATE SENSOR ID
        
        if(verbose)
            std::cout << "--------------------\n Current name: " <<
            name_data_map.first << "\n";
        
        // Set the ns read query
        ns_read_struct ns_read =
        {
            .bucket = bucket_,
            .machine_name = epitrend_machine_name,
            .sensor_name = name_data_map.first
        }; 
        ns_read.set_read_query();
        
        // Query the ns table
        std::string response;
        queryData2(response, ns_read.read_query);
        
        // Parse the response
        std::vector<std::unordered_map<std::string,std::string>> parsed_response = parseQueryResponse(response);
        
        // Prepare the sensor id associated with the current sensor name
        int valid_sensor_id = -1;

        // Check if data is found
        if(parsed_response.size() == 0) {
            if(verbose) std::cout << "No entry found for sensor: " << name_data_map.first << "\n";

            // Set the ns read all data query
            ns_read_all_struct ns_read_all = {.bucket = bucket_};
            ns_read_all.set_read_query();

            // Query the ns table for all data
            response = "";
            queryData2(response, ns_read_all.read_query);

            if(verbose) std::cout << "Query: " << ns_read_all.read_query << "\n";
            if(verbose) std::cout << "Response: " << response << "\n";

            // Parse the response
            parsed_response = parseQueryResponse(response);

            // Check if parse is empty to prevent segmentation faults
            if (parsed_response.empty()) {
                // Manually set first sensor id since there no data found within the name-series (ns). New table?
                if(verbose) std::cerr << "Warning in InfluxDatabase::copyEpitrendToBucket call: "
                "parsed response is empty due to empty name-series table.\n";
                valid_sensor_id = 1;

            } else {
                // Grab all the sensor_id values
                std::vector<int> sensor_ids;
                for(std::unordered_map<std::string,std::string>& element : parsed_response) {
                    try {
                        sensor_ids.push_back(stoi(element["_value"]));
                    }
                    catch (std::exception& e) {
                        std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket: error parsing sensor_id\n";
                        throw std::runtime_error("Error in InfluxDatabase::copyEpitrendToBucket: error parsing sensor_id\n");
                    }
                    if(verbose) std::cout << "Found existing sensor_id: " << stoi(element["_value"]) << "\n";
                }

                // Get the next sensor_id
                valid_sensor_id = *std::max_element(sensor_ids.begin(), sensor_ids.end()) + 1;
                if(verbose) std::cout << "Next sensor_id available for \"" << name_data_map.first <<"\": " << valid_sensor_id << "\n";

            }

            // Set the ns write query
            ns_write_struct ns_write = 
            {
                .machine_name = epitrend_machine_name,
                .sensor_name = name_data_map.first,
                .sensor_id = std::to_string(valid_sensor_id)
            };
            ns_write.set_write_query();
            if(verbose) std::cout << "Write query: " << ns_write.write_query << "\n";

            // Write the new sensor_id with the machine and sensor name into ns
            writeBatchData2({ns_write.write_query}, verbose);

        } else {
            if(verbose) std::cout << "Entry found for sensor: " << name_data_map.first << "\n";
            
            // Get the sensor_id
            valid_sensor_id = stoi(parsed_response[0]["_value"]);
            if(verbose) std::cout << "Sensor_id: " << valid_sensor_id << "\n";

        }

        // Prepare ts query write statement
        ts_write_struct ts_write = 
        {
            .sensor_id = std::to_string(valid_sensor_id),
        };

        // Loop through all the time-value pairs for the current name
        std::vector<std::string> batch_data;
        for (const auto& time_value : name_data_map.second) {
            // Prepare the ts write query
            ts_write.num = std::to_string(time_value.second);
            ts_write.timestamp = std::to_string(convertDaysFromEpochToPrecisionFromUnix(time_value.first));
            ts_write.set_write_query();
            
            // Batch the data
            batch_data.push_back(ts_write.write_query);

            if(batch_data.size() >= batchSize) {
                // Write the time-value pair to the ts table
                if(verbose) std::cout << "Writing batch data...\n";
                writeBatchData2(batch_data, false);
                batch_data.clear();
            }
        }
        
        // Write the remaining data
        if(batch_data.size() > 0) {
            if(verbose) std::cout << "Writing batch data...\n";
            writeBatchData2(batch_data, verbose);
        }

    }
    return true;
}

bool InfluxDatabase::copyEpitrendToBucket2(EpitrendBinaryData data, bool verbose){
    // Batch size
    const int batchSize = 5000;

    // Number of retry calls
    const int retryCalls = 5;

    const std::string epitrend_machine_name = "GEN200";

    // Prepare time-series (ts) query write statement e.g.
    //measurement + ",sensor_id_=1 num=299i 1735728000000";
    struct ts_write_struct {
        std::string sensor_id;
        std::string num;  // MUST BE A DECIMAL
        std::string timestamp;
        std::string write_query;
        void set_write_query(){write_query ="ts,sensor_id_=" + 
            sensor_id + " num=" + 
            num + " " + timestamp;
        }
    };

    // Prepare name-series (ns) query write statement e.g.
    //measurement + ",machine_=\"machine.name.2\",sensor_=\"sensor.name.2\" sensor_id=\"4\" " + std::to_string(default_ns_timestamp);
    struct ns_write_struct {
        std::string machine_name;
        std::string sensor_name;
        std::string sensor_id;
        std::string default_timestamp = "2000000000000";
        std::string write_query;
        void set_write_query(){write_query = "ns,machine_=" + escapeSpecialChars(machine_name) + 
            ",sensor_=" + escapeSpecialChars(sensor_name) + 
            " sensor_id=\"" + sensor_id + 
            "\" " + default_timestamp;
        }
    };

    // Prepare name-series (ns) query read statement
    //     "from(bucket: \"test-bucket\")"
    //   "|> range(start: -100y, stop: 50y)"
    //   "|> filter(fn: (r) => r[\"_measurement\"] == \"ns\")"
    ;
    struct ns_read_struct {
        std::string bucket;
        std::string machine_name;
        std::string sensor_name;
        std::string read_query;
        void set_read_query(){read_query = "from(bucket: \"" + bucket + "\") "
            "|> range(start: -50y, stop: 100y)"
            "|> filter(fn: (r) => r[\"_measurement\"] == \"ns\")" 
            "|> filter(fn: (r) => r[\"sensor_\"] == \"" + sensor_name + "\")"
            "|> filter(fn: (r) => r[\"machine_\"] == \"" + machine_name + "\")";
        }
    };

    // Prepare name-series (ns) query read all data statement
    struct ns_read_all_struct {
        std::string bucket;
        std::string read_query;
        void set_read_query(){read_query = "from(bucket: \"" + bucket + "\") "
            "|> range(start: -50y, stop: 100y)"
            "|> filter(fn: (r) => r[\"_measurement\"] == \"ns\")";
        }
    };

    // Set the ns read all data query
    ns_read_all_struct ns_read_all = {.bucket = bucket_};
    ns_read_all.set_read_query();

    // Read the ns table for all data
    std::string response;
    response = "";
    queryData2(response, ns_read_all.read_query);

    // Parse the response
    std::vector<std::unordered_map<std::string,std::string>> parsed_response = parseQueryResponse(response);

    // Cache all the sensor-name and sensor-id pairs that exist in the ns table
    std::unordered_map<std::string, std::string> sensor_names_to_ids;
    for(const auto& element : parsed_response) {
        // Check sensor_ and sensor_id_ keys exist (ns table should contain these keys)
        if(element.find("sensor_") == element.end() || element.find("_value") == element.end()) {
            std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: "
            "sensor_ or sensor_id key not found in ns table\n";
            throw std::runtime_error("Error in InfluxDatabase::copyEpitrendToBucket2 call: "
            "sensor_ or sensor_id key not found in ns table\n");
        }

        // Cache the sensor-name and sensor-id pairs
        sensor_names_to_ids[element.at("sensor_")] = element.at("_value");
        //std::cout << "Cached sensor-name: " << element.at("sensor_") << " with sensor-id: " << element.at("_value") << "\n";
    }

    // Loop through all data
    std::unordered_map<std::string, std::unordered_map<double,double>> raw_data = data.getAllTimeSeriesData();
    std::vector<std::string> batch_data;

    for(const auto& name_data_map : raw_data) {
        // CHECK IF PART NAME IS IN NS TABLE
        // IF IT ISN'T
            // ADD ENTRY OF MACHINE NAME AND PART NAME INTO TABLE
            // GET NEW SENSOR_ID FOR NAME AND ALSO INTO TABLE
        // IF IT IS
            // GET THE SENSOR_ID
        // ENTER ALL ASSOCIATED DATA INTO TS TABLE WITH ASSOCIATE SENSOR ID
        
        if(verbose)
            std::cout << "--------------------\n Current name: " <<
            name_data_map.first << "\n";

        // Prepare the sensor id associated with the current sensor name
        int valid_sensor_id = -1;

        // Check if sensor_ name exists in the ns table
        int found_sensor_name_in_ns = 
            !(sensor_names_to_ids.find(escapeSpecialChars(name_data_map.first)) == sensor_names_to_ids.end());

        // Check if data is found
        if(!found_sensor_name_in_ns) {
            if(verbose) std::cout << "No entry found for sensor: " << name_data_map.first << "\n";

            // Check if cache is empty to prevent segmentation faults
            if (sensor_names_to_ids.empty()) {
                // Manually set first sensor id since there no data found within the name-series (ns). New table?
                if(verbose) std::cerr << "Warning in InfluxDatabase::copyEpitrendToBucket call: "
                "no data found in ns table.\n";
                valid_sensor_id = 1;

            } else {
                // Get the next sensor_id
                valid_sensor_id = std::stoi(std::max_element(sensor_names_to_ids.begin(), sensor_names_to_ids.end(),
                    [](const auto& a, const auto& b) {
                        return std::stoi(a.second) < std::stoi(b.second);
                    })->second) + 1;

                if(verbose) std::cout << "Next sensor_id available for \"" << name_data_map.first <<"\": " << valid_sensor_id << "\n";

            }

            // Set the ns write query
            ns_write_struct ns_write = 
            {
                .machine_name = epitrend_machine_name,
                .sensor_name = name_data_map.first,
                .sensor_id = std::to_string(valid_sensor_id)
            };
            ns_write.set_write_query();
            if(verbose) std::cout << "Write query: " << ns_write.write_query << "\n";

            // Write the new sensor_id with the machine and sensor name into ns
            writeBatchData2({ns_write.write_query}, verbose);

            // Update the cache
            sensor_names_to_ids[name_data_map.first] = std::to_string(valid_sensor_id);

        } else {
            if(verbose) std::cout << "Entry found for sensor: " << name_data_map.first << "\n";

            // Get the sensor_id
            valid_sensor_id = std::stoi(sensor_names_to_ids.at(name_data_map.first));
            if(verbose) std::cout << "Sensor_id: " << valid_sensor_id << "\n";

        }

        // Prepare ts query write statement
        ts_write_struct ts_write = 
        {
            .sensor_id = std::to_string(valid_sensor_id),
        };

        // Loop through all the time-value pairs for the current name
        for (const auto& time_value : name_data_map.second) {
            // Prepare the ts write query
            std::ostringstream num_stream;
            num_stream << std::setprecision(15) << time_value.second;
            ts_write.num = num_stream.str();

            std::ostringstream timestamp_stream;
            timestamp_stream << std::setprecision(15) << convertDaysFromEpochToPrecisionFromUnix(time_value.first);
            ts_write.timestamp = timestamp_stream.str();

            ts_write.set_write_query();
            
            // Batch the data
            batch_data.push_back(ts_write.write_query);

            if(batch_data.size() >= batchSize) {
                // Write the time-value pair to the ts table
                if(verbose) std::cout << "Writing batch data...\n";
                
                for(int i = 0; i < retryCalls; i++){
                    try {
                        writeBatchData2(batch_data, false);
                        break;
                    } catch (std::exception& e) {
                        if(verbose) std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: error writing to ts table\n";
                        if(verbose) std::cerr << "Error message: " << e.what() << "\n";
                        if(verbose) std::cerr << "Retrying...\n";
                        if (i == 9) {
                            if(verbose) std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: failed to write to ts table after " << retryCalls << " attempts\n";
                            throw std::runtime_error("Error in InfluxDatabase::copyEpitrendToBucket2 call: failed to write to ts table\n");
                        }
                        std::this_thread::sleep_for(std::chrono::seconds(1));

                    }
                }
                // writeBatchData2(batch_data, false);
                batch_data.clear();
            }
        }
        
    }

    // Write the remaining data
    if(batch_data.size() > 0) {
        if(verbose) std::cout << "Writing batch data...\n";

        for(int i = 0; i < retryCalls; i++){
            try {
                writeBatchData2(batch_data, false);
                break;
            } catch (std::exception& e) {
                if(verbose) std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: error writing remaining data to ts table\n";
                if(verbose) std::cerr << "Error message: " << e.what() << "\n";
                if(verbose) std::cerr << "Batch size is: " << batch_data.size() << "\n";
                if(verbose) std::cerr << "Retrying...\n";
                if (i == 9) {
                    if(verbose) std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: failed to write to ts table after " << retryCalls << " attempts\n";
                    throw std::runtime_error("Error in InfluxDatabase::copyEpitrendToBucket2 call: failed to write to ts table\n");
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));

            }
        }
        // writeBatchData2(batch_data, verbose);
    }

    return true;
}

bool InfluxDatabase::copyRGADataToBucket(RGAData data, bool verbose) {
    // Batch size
    const int batchSize = 5000;

    // Number of retry calls
    const int retryCalls = 5;

    const std::string epitrend_machine_name = "GEN200_RGA";


    // Prepare time-series (ts) query write statement e.g.
    //measurement + ",sensor_id_=1 num=299i 1735728000000";
    struct ts_write_struct {
        std::string sensor_id;
        std::string num;  // MUST BE A DECIMAL
        std::string timestamp;
        std::string write_query;
        void set_write_query(){write_query ="ts,sensor_id_=" + 
            sensor_id + " num=" + 
            num + " " + timestamp;
        }
    };

    // Prepare name-series (ns) query write statement e.g.
    //measurement + ",machine_=\"machine.name.2\",sensor_=\"sensor.name.2\" sensor_id=\"4\" " + std::to_string(default_ns_timestamp);
    struct ns_write_struct {
        std::string machine_name;
        std::string sensor_name;
        std::string sensor_id;
        std::string default_timestamp = "2000000000000";
        std::string write_query;
        void set_write_query(){write_query = "ns,machine_=" + escapeSpecialChars(machine_name) + 
            ",sensor_=" + escapeSpecialChars(sensor_name) + 
            " sensor_id=\"" + sensor_id + 
            "\" " + default_timestamp;
        }
    };

    // Prepare name-series (ns) query read statement
    //     "from(bucket: \"test-bucket\")"
    //   "|> range(start: -100y, stop: 50y)"
    //   "|> filter(fn: (r) => r[\"_measurement\"] == \"ns\")"
    ;
    struct ns_read_struct {
        std::string bucket;
        std::string machine_name;
        std::string sensor_name;
        std::string read_query;
        void set_read_query(){read_query = "from(bucket: \"" + bucket + "\") "
            "|> range(start: -50y, stop: 100y)"
            "|> filter(fn: (r) => r[\"_measurement\"] == \"ns\")" 
            "|> filter(fn: (r) => r[\"sensor_\"] == \"" + sensor_name + "\")"
            "|> filter(fn: (r) => r[\"machine_\"] == \"" + machine_name + "\")";
        }
    };

    // Prepare name-series (ns) query read all data statement
    struct ns_read_all_struct {
        std::string bucket;
        std::string read_query;
        void set_read_query(){read_query = "from(bucket: \"" + bucket + "\") "
            "|> range(start: -50y, stop: 100y)"
            "|> filter(fn: (r) => r[\"_measurement\"] == \"ns\")";
        }
    };

    // Set the ns read all data query
    ns_read_all_struct ns_read_all = {.bucket = bucket_};
    ns_read_all.set_read_query();

    // Read the ns table for all data
    std::string response;
    response = "";
    queryData2(response, ns_read_all.read_query);

    // Parse the response
    std::vector<std::unordered_map<std::string,std::string>> parsed_response = parseQueryResponse(response);

    // Cache all the sensor-name and sensor-id pairs that exist in the ns table
    std::unordered_map<std::string, std::string> sensor_names_to_ids;
    for(const auto& element : parsed_response) {
        // Check sensor_ and sensor_id_ keys exist (ns table should contain these keys)
        if(element.find("sensor_") == element.end() || element.find("_value") == element.end()) {
            std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: "
            "sensor_ or sensor_id key not found as an entry into ns table\n";
            throw std::runtime_error("Error in InfluxDatabase::copyEpitrendToBucket2 call: "
            "sensor_ or sensor_id key not found as an entry into ns table\n");
        }

        // Cache the sensor-name and sensor-id pairs
        sensor_names_to_ids[element.at("sensor_")] = element.at("_value");
        //std::cout << "Cached sensor-name: " << element.at("sensor_") << " with sensor-id: " << element.at("_value") << "\n";
    }

    // Loop through all data
    const auto& raw_data = data.getAllTimeSeriesData();
    std::vector<std::string> batch_data;

    for(const auto& name_data_map : raw_data) {
        // CHECK IF PART NAME IS IN NS TABLE
        // IF IT ISN'T
            // ADD ENTRY OF MACHINE NAME AND PART NAME INTO TABLE
            // GET NEW SENSOR_ID FOR NAME AND ALSO INTO TABLE
        // IF IT IS
            // GET THE SENSOR_ID
        // ENTER ALL ASSOCIATED DATA INTO TS TABLE WITH ASSOCIATE SENSOR ID
        
        std::string name = "RGA." + name_data_map.first.binsString();

        if(verbose)
            std::cout << "--------------------\n Current name: " <<
            name << "\n";

        // Prepare the sensor id associated with the current sensor name
        int valid_sensor_id = -1;

        // Check if sensor_ name exists in the ns table
        int found_sensor_name_in_ns = 
            !(sensor_names_to_ids.find(escapeSpecialChars(name)) == sensor_names_to_ids.end());

        // Check if data is found
        if(!found_sensor_name_in_ns) {
            if(verbose) std::cout << "No entry found for sensor: " << name << "\n";

            // Check if cache is empty to prevent segmentation faults
            if (sensor_names_to_ids.empty()) {
                // Manually set first sensor id since there no data found within the name-series (ns). New table?
                if(verbose) std::cerr << "Warning in InfluxDatabase::copyEpitrendToBucket call: "
                "no data found in ns table.\n";
                valid_sensor_id = 1;

            } else {
                // Get the next sensor_id
                valid_sensor_id = std::stoi(std::max_element(sensor_names_to_ids.begin(), sensor_names_to_ids.end(),
                    [](const auto& a, const auto& b) {
                        return std::stoi(a.second) < std::stoi(b.second);
                    })->second) + 1;

                if(verbose) std::cout << "Next sensor_id available for \"" << name <<"\": " << valid_sensor_id << "\n";

            }

            // Set the ns write query
            ns_write_struct ns_write = 
            {
                .machine_name = epitrend_machine_name,
                .sensor_name = name,
                .sensor_id = std::to_string(valid_sensor_id)
            };
            ns_write.set_write_query();
            if(verbose) std::cout << "Write query: " << ns_write.write_query << "\n";

            // Write the new sensor_id with the machine and sensor name into ns
            writeBatchData2({ns_write.write_query}, verbose);

            // Update the cache
            sensor_names_to_ids[escapeSpecialChars(name)] = std::to_string(valid_sensor_id);

        } else {
            if(verbose) std::cout << "Entry found for sensor: " << name << "\n";

            // Get the sensor_id
            valid_sensor_id = std::stoi(sensor_names_to_ids.at(name));
            if(verbose) std::cout << "Sensor_id: " << valid_sensor_id << "\n";

        }

        // Prepare ts query write statement
        ts_write_struct ts_write = 
        {
            .sensor_id = std::to_string(valid_sensor_id),
        };

        // Define the stringstream for precision
        std::ostringstream timestamp_stream;
        timestamp_stream.precision(15);
        timestamp_stream << std::fixed;
        std::ostringstream num_stream;
        num_stream.precision(15);
        num_stream << std::fixed;

        // Loop through all the time-value pairs for the current name
        for (const auto& time_value : name_data_map.second) {
            // Prepare the ts write query
            num_stream.str("");
            num_stream << time_value.second;
            ts_write.num = num_stream.str();

            timestamp_stream.str("");
            timestamp_stream << convertSecondsFromUnixToPrecisionFromUnix(time_value.first);
            ts_write.timestamp = timestamp_stream.str();

            ts_write.set_write_query();
            
            // Batch the data
            batch_data.push_back(ts_write.write_query);

            // Print query
            //if(verbose) std::cout << "Query: " << ts_write.write_query << "\n";

            if(batch_data.size() >= batchSize) {
                // Write the time-value pair to the ts table
                if(verbose) std::cout << "Writing batch data...\n";
                
                for(int i = 0; i < retryCalls; i++){
                    try {
                        writeBatchData2(batch_data, false);
                        break;
                    } catch (std::exception& e) {
                        if(verbose) std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: error writing to ts table\n";
                        if(verbose) std::cerr << "Error message: " << e.what() << "\n";
                        if(verbose) std::cerr << "Retrying...\n";
                        if (i == 9) {
                            if(verbose) std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: failed to write to ts table after " << retryCalls << " attempts\n";
                            throw std::runtime_error("Error in InfluxDatabase::copyEpitrendToBucket2 call: failed to write to ts table\n");
                        }
                        std::this_thread::sleep_for(std::chrono::seconds(1));

                    }
                }
                // writeBatchData2(batch_data, false);
                batch_data.clear();
            }
        }
        
    }

    // Write the remaining data
    if(batch_data.size() > 0) {
        if(verbose) std::cout << "Writing batch data...\n";

        for(int i = 0; i < retryCalls; i++){
            try {
                writeBatchData2(batch_data, false);
                break;
            } catch (std::exception& e) {
                if(verbose) std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: error writing remaining data to ts table\n";
                if(verbose) std::cerr << "Error message: " << e.what() << "\n";
                if(verbose) std::cerr << "Batch size is: " << batch_data.size() << "\n";
                if(verbose) std::cerr << "Retrying...\n";
                if (i == 9) {
                    if(verbose) std::cerr << "Error in InfluxDatabase::copyEpitrendToBucket2 call: failed to write to ts table after " << retryCalls << " attempts\n";
                    throw std::runtime_error("Error in InfluxDatabase::copyEpitrendToBucket2 call: failed to write to ts table\n");
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));

            }
        }
        // writeBatchData2(batch_data, verbose);
    }

    return true;
}