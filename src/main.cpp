#include "Common.hpp"
#include "FileReader.hpp"
#include "Config.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"
#include "AzureDatabase.hpp"
#include "InfluxDatabase.hpp"
#include "influxdb.hpp"



int main() {
const long int default_machine_timestamp = 1643840542000;
const std::string& target_org = "au-eng-mbe";
const std::string& target_bucket = "test-bucket";
const std::string& machine_part_db = "machine-db";
const std::string& host = "127.0.0.1";
const int port = 8086;
const std::string& org = target_org;
const std::string& db = target_bucket;
const std::string& user = "";
const std::string& password = "";
const std::string& precision = "ms";
const std::string& token = "lZ16pxdtlcFQxE0n2gQ5u64yy-uuWS1HDp0UUQzqZMBtue64_CFZKDqpiHoSk5npY4Q5Bk6kIqToGTVwxx5nQw==";

influxdb_cpp::server_info si("127.0.0.1", 
    8086, 
    target_bucket, 
    "", 
    "", 
    "",
    "lZ16pxdtlcFQxE0n2gQ5u64yy-uuWS1HDp0UUQzqZMBtue64_CFZKDqpiHoSk5npY4Q5Bk6kIqToGTVwxx5nQw==");

//-------------------------Section for testing influxDatabase wrapper class-------------------------
// InfluxDatabase influx_db(host, port, org, db, user, password, precision, token);

// // Check the health of the connection
// influx_db.checkConnection(true);

// // Bulk writing data
// std::vector<std::string> data_points;
// // data_points.push_back("ts,machine_id=1,part_id=3 value=200i");
// data_points.push_back("ts,machine_id=2,part_id=1 value=201i 1642147989041");
// data_points.push_back("ts,machine_id=2,part_id=1 value=201i 1642147989042");

// WriteResult result = influx_db.writeBulkData(data_points, true);
// if (result.success) {
//     std::cout << "Bulk data written successfully." << std::endl;
// } else {
//     std::cout << "Failed to write bulk data. Error: " << result.error_message << std::endl;
//     std::cout << "Failed data point: " << result.data_out << std::endl;
// }

// // Test the query function
// std::string query = "from(bucket: \"test-bucket\") |> range(start: -10y) |> filter(fn: (r) => r[\"machine_id\"] == \"2\")";
// std::string response = influx_db.queryData2(query, true);
// std::cout << "Response from query: " << response << std::endl;

// // Parse the query response
// std::vector<std::unordered_map<std::string, std::string>> parsed_data = influx_db.parseQueryResult(response);
// for (const auto& row : parsed_data) {
//     for (const auto& [key, value] : row) {
//         std::cout << key << ": " << value << " | ";
//     }
//     std::cout << std::endl;
// }


//-------------------------Section for testing parsing the machine part database------------------------

// Initializing machine part database
InfluxDatabase influx_db_machine_part(host, port, org, machine_part_db, user, password, precision, token);

// Check the health of the connection
influx_db_machine_part.checkConnection(true);

std::vector<std::string> data_points;
data_points.push_back("ns,name=machine.part1.name machine_id=1i,part_id=1i " + std::to_string(default_machine_timestamp));

WriteResult result = influx_db_machine_part.writeBulkData(data_points, true);
if (result.success) {
    std::cout << "Bulk data written successfully." << std::endl;
    std::cout << "Inputted data points: " << result.data_out << std::endl;
} else {
    std::cout << "Failed to write bulk data. Error: " << result.error_message << std::endl;
    std::cout << "Failed data point: " << result.data_out << std::endl;
}

std::string query = "from(bucket: \"machine-db\") |> range(start: -100y)"
" |> filter(fn: (r) => r[\"_measurement\"] == \"nspsp\")";

std::string response = influx_db_machine_part.queryData2(query, true);
std::cout << "Response from query: " << response << std::endl;

// Return a map of <names,{machine_id,part_id}>
// If new names are found, 

return 0;
}


