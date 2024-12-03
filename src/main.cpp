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
const std::string& target_org = "terminal";
const std::string& target_bucket = "test-bucket";
const std::string& host = "127.0.0.1";
const int port = 8086;
const std::string& db = target_bucket;
const std::string& user = "terminal";
const std::string& password = "$Newmonogatar1";
const std::string& precision = "ms";
const std::string& token = "E50icKBWaeccyZRwfdYDTtYxm10cHRPp8NRY0mp0upeEDZJC_STQfmJJzoBK9qCPm6mVUR9FhzysNlemzEmsOw==";

influxdb_cpp::server_info si("127.0.0.1", 
    8086, 
    target_bucket, 
    "", 
    "", 
    "",
    "E50icKBWaeccyZRwfdYDTtYxm10cHRPp8NRY0mp0upeEDZJC_STQfmJJzoBK9qCPm6mVUR9FhzysNlemzEmsOw==");

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
InfluxDatabase influx_db(host, port, db, user, password, precision, token);

// Check the health of the connection
influx_db.checkConnection(true);

// Disconnect influx object
influx_db.disconnect(true);

// Check that disconnection was successful
influx_db.checkConnection(true);

// // Connect influx object
// influx_db.connect(host, port, db, user, password, precision, token); 

// // Check the health of the connection
// influx_db.checkConnection(true);

// // Writing data into db
// const std::string measurement = "ts";
// const std::string tags = "machine_id=1,part_id=1";
// const std::string fields = "value=199";
// long long timestamp = 1735728000000; // Timestamp for 2024-Nov-01 00:00:00.000 in milliseconds

// if (influx_db.writeData(measurement, tags, fields, timestamp, true)) {
//     std::cout << "Data written successfully." << std::endl;
// } else {
//     std::cout << "Failed to write data." << std::endl;
// }

// Trying writing data straight with the influxdb_cpp class
influxdb_cpp::builder()
    .meas("ts")
    .tag("machine_id", "1")
    .tag("part_id", "2")
    .field("value", 199)
    .post_http(si);

return 0;
}


