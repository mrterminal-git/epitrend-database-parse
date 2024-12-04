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


return 0;
}


