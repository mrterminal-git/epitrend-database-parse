#include "Common.hpp"
#include "FileReader.hpp"
#include "Config.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"
#include "AzureDatabase.hpp"
#include "InfluxDatabase.hpp"
#include "influxdb.hpp"
#include <curl/curl.h>

int main() {
const std::string& org = "terminal";
const std::string& host = "127.0.0.1";
const int port = 8086;
const std::string& bucket = "test-bucket";
const std::string& user = "";
const std::string& password = "";
const std::string& precision = "ms";
const std::string& token = "E50icKBWaeccyZRwfdYDTtYxm10cHRPp8NRY0mp0upeEDZJC_STQfmJJzoBK9qCPm6mVUR9FhzysNlemzEmsOw==";

// influxdb_cpp::server_info si("127.0.0.1", 
//     8086, 
//     bucket, 
//     "", 
//     "", 
//     "",
//     "E50icKBWaeccyZRwfdYDTtYxm10cHRPp8NRY0mp0upeEDZJC_STQfmJJzoBK9qCPm6mVUR9FhzysNlemzEmsOw==");

// std::string response;
// std::string query_show_database = "SHOW DATABASES";
// std::string query_select_all = "SELECT * FROM \"test-bucket\".\"autogen\".\"foo\"";

// int result = influxdb_cpp::query(response, query_select_all, si);
// if (result != 0) {
//     std::cerr << "Failed to connect to InfluxDB: " << response << std::endl;
//     throw std::runtime_error("Failed to connect to InfluxDB: " + response);
// }

// std::cout << response << "\n";

// // Query with a filter
// std::string filter = "r._measurement == \"foo\" and r.tag_key == \"tag_value\"";
// result = influxdb_cpp::query_with_filter(response, target_bucket, target_org, filter, si);
// if (result != 0) {
//     std::cerr << "Failed to query InfluxDB with filter" << std::endl;
//     throw std::runtime_error("Failed to query InfluxDB with filter");
// }

// std::cout << response << "\n";  

// // influxdb_cpp::builder()
// //     .meas("foo")
// //     .field("x", 20)
// //     .post_http(si);

//-------------------------Section for testing influxDatabase wrapper class-------------------------
// Create influx object
InfluxDatabase influx_db(host, port, org, bucket, user, password, precision, token);

// Check the health of the connection
influx_db.checkConnection(true);

// Writing data into db
const std::string measurement = "ts";
const std::string tags = "machine_id=1,part_id=1";
const std::string fields = "value=199";
long long timestamp = 1735728000000; // Timestamp for 2024-Nov-01 00:00:00.000 in milliseconds
long long default_ns_timestamp = 2000000000000;

// std::vector<std::string> data_points;
// data_points.push_back(measurement + ",sensor_id_=1 num=299i 1735728000000");
// data_points.push_back(measurement + ",sensor_id_=2 num=199i 1735728000000");

// data_points.push_back(measurement + ",machine_=\"machine.name.1\",sensor_=\"sensor.name.1\" sensor_id=\"1\" " + std::to_string(default_ns_timestamp));
// data_points.push_back(measurement + ",machine_=\"machine.name.2\",sensor_=\"sensor.name.1\" sensor_id=\"2\" " + std::to_string(default_ns_timestamp));
// data_points.push_back(measurement + ",machine_=\"machine.name.1\",sensor_=\"sensor.name.2\" sensor_id=\"3\" " + std::to_string(default_ns_timestamp));
// data_points.push_back(measurement + ",machine_=\"machine.name.2\",sensor_=\"sensor.name.2\" sensor_id=\"4\" " + std::to_string(default_ns_timestamp));

// influx_db.writeBatchData2(data_points, true);

//-------------------------Section for testing influxDatabase wrapper class: query-------------------------
const std::string query = "from(bucket: \"test-bucket\")"
  "|> range(start: -100y, stop: 50y)"
  "|> filter(fn: (r) => r[\"_measurement\"] == \"ts\")";

std::string response;
influx_db.queryData2(response, query);   
std::cout << "Query: " << query << "\n";
std::cout << "Response: " << response << "\n";

std::vector<std::unordered_map<std::string,std::string>> parsed_response = influx_db.parseQueryResponse(response);
for(const auto& element : parsed_response){
    for (const auto& element_inner : element){ 
        std::cout << element_inner.first << "==" << element_inner.second << "||";
    }
    std::cout << "\n";
}

EpitrendBinaryData binary_data;
std::ofstream epitrend_finished_file(Config::getOutputDir + "times_inserted_into_SQL_server.txt", std::ios_base::app);
std::string times_finished_string;
int year;
year = 2024;
for (int month = 1; month < 13; month++) {
for (int day = 1; day < 32; day++) { 
for (int hour = 0; hour < 25; hour++) {
    try {
        // Parse the Epitrend binary data file
        FileReader::parseEpitrendBinaryDataFile(binary_data,year,month,day,hour,false);
    
    } catch ( std::exception& e) {
        // Catching errors due to times that exist
        std::cout << "Error occured for " << year << "," << month << "," << day << "," << hour << "\n" << e.what() << "\n";

    }
    
    // Log the parsed file
    times_finished_string += std::to_string(year) + "," + std::to_string(month) + "," + std::to_string(day) + std::to_string(hour) + "\n";
    
    // Check the current size of the epitrend binary data object
    std::cout << "Current size of EpitrendBinaryData object: " << binary_data.getByteSize() << "\n";
    if (binary_data.getByteSize() > 0.1 * pow(10.0, 6.0) ) { // Limit INSERTS to 50 mb packs
        std::cout << "Curret epitrend data object exceeded size limit -> inserting data into SQL DB and flushing object...\n";
        
        // Insert data into SQL and flush current epitrend data
        binary_data.printFileAllTimeSeriesData("temp.txt");

        // Copy all data into SQL table
        // ===================AZURE SQL VERSION===================
        // auto start = std::chrono::system_clock::now();
        // db.copyToSQL2("epitrend_data_time_series", binary_data);
        // auto end = std::chrono::system_clock::now();
        // auto elapsed =
        //     std::chrono::duration_cast<std::chrono::seconds>(end - start);  
        // std::cout << elapsed.count() << '\n';

        // ===================INFLUXDB VERSION===================
        // CHECK IF PART NAME IS IN NS TABLE
        // IF IT ISN'T
            // ADD ENTRY OF MACHINE NAME AND PART NAME INTO TABLE
            // GET NEW SENSOR_ID FOR NAME AND ALSO INTO TABLE
        // IF IT IS
            // GET THE SENSOR_ID
        // ENTER DATA INTO TS TABLE WITH ASSOCIATE SENSOR ID

        // Flush the current epitrend data object
        binary_data.clear();
        
        // Insert the completed times into log and flush
        epitrend_finished_file << times_finished_string;
        times_finished_string = "";
        
    }
}
}
}
epitrend_finished_file.close();




return 0;
}


