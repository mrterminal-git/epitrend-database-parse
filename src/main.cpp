#include "Common.hpp"
#include "FileReader.hpp"
#include "Config.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"
#include "AzureDatabase.hpp"

int main() {

std::string connectionString = 
    "Driver=ODBC Driver 18 for SQL Server;"
    "Server=tcp:epitrenddbts.database.windows.net,1433;"
    "Database=EpitrendDBTS;"
    "Uid=mrterminal;"
    "Pwd=$Newmonogatar1;"
    "Encrypt=yes;"
    "TrustServerCertificate=no;"
    "Connection Timeout=30;";
std::string query_table_name = "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE = 'BASE TABLE';";
std::string query_create_table = "CREATE TABLE my_first_table (column_1 INT, column_2 DEC(10,0), column_3 INT)";
std::string query_table_contents = "SELECT COLUMN_NAME, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH "
                                   "FROM INFORMATION_SCHEMA.COLUMNS "
                                   "WHERE TABLE_NAME = 'my_first_table';";

if (AzureDatabase::queryDatabase(connectionString, query_table_contents)) {
    std::cout << "Query executed successfully." << std::endl;
} else {
    std::cerr << "Query execution failed." << std::endl;
}

// Testing parsing the binary file using the binary format file
// EpitrendBinaryData object
// using the EpitrendBinaryFormat object, extract the EpitrendData object

// EpitrendBinaryData binaryData;
// binaryData.addDataItem("Parameter1", {0.1, 2});
// binaryData.addDataItem("Parameter1", {0.2, 3});
// binaryData.addDataItem("Parameter1", {0.1, 1});
// binaryData.addDataItem("Parameter2", {0.1, 2});

// binaryData.printAllTimeSeriesData();

EpitrendBinaryData binary_data;
for (int i = 0; i < 24; i++) {
    FileReader::parseEpitrendBinaryDataFile(binary_data,2024,11,1,i,false);
    std::cout << "Current size of EpitrendBinaryData object: " << binary_data.getByteSize() << "\n";
}
binary_data.printFileAllTimeSeriesData("temp.txt");


return 0;
}


