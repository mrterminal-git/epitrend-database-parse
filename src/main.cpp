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

EpitrendBinaryData binary_data;
std::ofstream epitrend_finished_file("times_inserted_into_SQL_server.txt", std::ios_base::app);
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
    if (binary_data.getByteSize() > 50 * pow(10.0, 6.0) ) { // Limit INSERTS to 50 mb packs
        std::cout << "Curret epitrend data object exceeded size limit -> inserting data into SQL DB and flushing object...\n";
        
        // Insert data into SQL and flush current epitrend data
        binary_data.printFileAllTimeSeriesData("temp.txt");

        // Copy all data into SQL table
        db.copyToSQL("epitrend_data_time_series", binary_data);

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


