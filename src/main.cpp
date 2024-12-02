#include "Common.hpp"
#include "FileReader.hpp"
#include "Config.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"
#include "AzureDatabase.hpp"
#include "InfluxDatabase.hpp"

int main() {

std::string query_table_name = "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE = 'BASE TABLE';";
std::string query_create_table = 
    "CREATE TABLE epitrend_data_3 ( "
    "id BIGINT NOT NULL IDENTITY(1,1), "
    "name VARCHAR(255), "
    "date_time DATETIME2, "
    "value FLOAT, "
    "PRIMARY KEY (id) );";
std::string query_table_contents = "SELECT COLUMN_NAME, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH "
                                   "FROM INFORMATION_SCHEMA.COLUMNS "
                                   "WHERE TABLE_NAME = 'epitrend_data_3';";
std::string query_table_all = "SELECT * FROM epitrend_data_3";
std::string query_table_delete_all = "DELETE FROM epitrend_data_3";

try {
    // Initialize InfluxDatabase instance
    InfluxDatabase db;

    // Test connect method
    std::cout << "Connecting to InfluxDB..." << std::endl;
    db.connect("127.0.0.1", 8086, "epitrend-ts-db", "", "", "ms", true);

    // Test connection status
    if (db.getConnectionStatus()) {
        std::cout << "Connection status: Connected" << std::endl;
    } else {
        std::cout << "Connection status: Not connected" << std::endl;
    }

    // Check if connection is healthy
    db.checkConnection(true);

    // Test writing a single data point
    std::cout << "Writing single data point..." << std::endl;
    db.writeData("test_measurement", "location=room1", "temperature=23.5", 0, true);

    // Test writing a batch of data points
    std::cout << "Writing batch data points..." << std::endl;
    std::vector<std::string> batchData = {
        "test_measurement,location=room1 temperature=24.1",
        "test_measurement,location=room2 temperature=22.8",
        "test_measurement,location=room3 temperature=25.0"
    };
    db.writeBatchData(batchData, true);

    // Query data
    std::cout << "Querying data..." << std::endl;
    std::string response = db.queryData("SELECT * FROM test_measurement", true);
    std::cout << "Raw query response:\n" << response << std::endl;

    // Parse the query result
    std::cout << "Parsing query response..." << std::endl;
    auto parsedResult = db.parseQueryResult(response);
    for (const auto& row : parsedResult) {
        for (const auto& [key, value] : row) {
            std::cout << key << ": " << value << " ";
        }
        std::cout << std::endl;
    }

    // Test disconnect method
    std::cout << "Disconnecting from InfluxDB..." << std::endl;
    db.disconnect(true);

    // Verify disconnection
    if (!db.getConnectionStatus()) {
        std::cout << "Successfully disconnected from InfluxDB." << std::endl;
    } else {
        std::cerr << "Disconnection failed!" << std::endl;
    }
} catch (const std::exception& e) {
    // Handle exceptions
    std::cerr << "An error occurred: " << e.what() << std::endl;
}


// EpitrendBinaryData binary_data;
// for (int i = 0; i < 4; i++) {
//     FileReader::parseEpitrendBinaryDataFile(binary_data,2024,11,1,i,false);
//     std::cout << "Current size of EpitrendBinaryData object: " << binary_data.getByteSize() << "\n";
// }
// binary_data.printFileAllTimeSeriesData("temp.txt");

// // copy all data into SQL table
// db.copyToSQL("epitrend_data_3", binary_data);

return 0;
}


