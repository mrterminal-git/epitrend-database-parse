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

influxdb_cpp::server_info si("127.0.0.1", 
    8086, 
    "test-bucket", 
    "", 
    "", 
    "", 
    "WjN4nU1NT2rVlb79Y8_3Mtp4aFPmXxfPVFn_rMYgFKvrN1XdRlzcZbS5hon7j8mcz2QrVgKeJEIt-YnkEBIbRg==");

std::string response;
std::string query_show_database = "SHOW DATABASES";
std::string query_select_all = "SELECT * FROM \"test-bucket\".\"autogen\".\"foo\"";

int result = influxdb_cpp::query(response, query_select_all, si);
if (result != 0) {
    std::cerr << "Failed to connect to InfluxDB: " << response << std::endl;
    throw std::runtime_error("Failed to connect to InfluxDB: " + response);
}

std::cout << response << "\n";

// Query with a filter
std::string filter = "r._measurement == \"foo\" and r.tag_key == \"tag_value\"";
result = influxdb_cpp::query_with_filter(response, "test-bucket", "au-mbe", filter, si);
if (result != 0) {
    std::cerr << "Failed to query InfluxDB with filter" << std::endl;
    throw std::runtime_error("Failed to query InfluxDB with filter");
}

std::cout << response << "\n";  

// influxdb_cpp::builder()
//     .meas("foo")
//     .field("x", 20)
//     .post_http(si);

return 0;
}


