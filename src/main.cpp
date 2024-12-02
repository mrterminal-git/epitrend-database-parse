#include "Common.hpp"
#include "FileReader.hpp"
#include "Config.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"
#include "AzureDatabase.hpp"

int main() {

std::string connectionString = 
    "Driver=ODBC Driver 18 for SQL Server;"
    "Server=tcp:au-eng-mbe.database.windows.net,1433;"
    "Database=TimeSeries_UVData;"
    "Uid=AU-Engineer-Admin;"
    "Pwd=f7rid4bTBd2JmeDWeVdH;"
    "Encrypt=yes;"
    "TrustServerCertificate=no;"
    "Connection Timeout=30;";

std::string query_table_name = "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE = 'BASE TABLE';";
std::string query_create_table = 
    "CREATE TABLE epitrend_data_time_series ( "
    "id BIGINT NOT NULL IDENTITY(1,1), "
    "name VARCHAR(255), "
    "date_time DATETIME2, "
    "value DECIMAL, "
    "PRIMARY KEY (id) );";
std::string query_table_contents = "SELECT COLUMN_NAME, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH "
                                   "FROM INFORMATION_SCHEMA.COLUMNS "
                                   "WHERE TABLE_NAME = 'epitrend_data_time_series';";
std::string query_table_all = "SELECT * FROM epitrend_data_time_series";
std::string query_table_delete_all = "DELETE FROM epitrend_data_time_series";

AzureDatabase db;

// Establish connection
if (!db.connect(connectionString)) {
    std::cerr << "Failed to connect to database.\n";
    return 1;
} else{
    std::cout << "Connection to database established!\n";
}

// Execute a query
if (db.queryExecute(query_table_contents)) {
    std::cout << "Query executed successfully!" << std::endl;
} else {
    std::cerr << "Query execution failed." << std::endl;
}

// EpitrendBinaryData binary_data;
// for (int i = 0; i < 4; i++) {
//     FileReader::parseEpitrendBinaryDataFile(binary_data,2024,11,1,i,false);
//     std::cout << "Current size of EpitrendBinaryData object: " << binary_data.getByteSize() << "\n";
// }
// binary_data.printFileAllTimeSeriesData("temp.txt");

// // copy all data into SQL table
// db.copyToSQL("epitrend_data_time_series", binary_data);

return 0;
}


