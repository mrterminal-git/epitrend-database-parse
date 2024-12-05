/*
 * main.cpp
 *
 * This program is designed to read binary data files from the Epitrend system,
 * process the data, and insert it into an InfluxDB database. The program iterates
 * over a specified range of years, months, days, and hours, attempting to read
 * and parse binary data files for each time period. If a file is found and successfully
 * parsed, the data is then copied into the InfluxDB database.
 *
 * The program uses several helper classes and libraries:
 * - Common.hpp: Contains common utility functions and definitions.
 * - FileReader.hpp: Provides functions to read and parse binary data files.
 * - Config.hpp: Manages configuration settings for the program.
 * - EpitrendBinaryFormat.hpp: Defines the format of the Epitrend binary data.
 * - EpitrendBinaryData.hpp: Represents the parsed binary data.
 * - AzureDatabase.hpp: Provides functions to interact with an Azure SQL database.
 * - InfluxDatabase.hpp: Provides functions to interact with an InfluxDB database.
 * - influxdb.hpp: Contains additional InfluxDB-related functions and definitions.
 * - curl/curl.h: Used for making HTTP requests to the InfluxDB server.
 *
 * The program performs the following steps:
 * 1. Define constants for the InfluxDB connection (organization, host, port, bucket, etc.).
 * 2. Create an InfluxDB object and check the health of the connection.
 * 3. Iterate over the specified range of years, months, days, and hours.
 * 4. For each time period, attempt to read and parse the corresponding binary data file.
 * 5. If the file is successfully parsed, copy the data into the InfluxDB database.
 * 6. Log the times when data is successfully inserted into the database.
 *
 * Note: The program assumes that the binary data files are named and organized
 * according to a specific convention based on the year, month, day, and hour.
 */

#include "Common.hpp"
#include "FileReader.hpp"
#include "Config.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"
#include "AzureDatabase.hpp"
#include "InfluxDatabase.hpp"
#include "influxdb.hpp"
#include <curl/curl.h>


// Define the constants
const std::string& org = "terminal";
const std::string& host = "127.0.0.1";
const int port = 8086;
const std::string& bucket = "test-bucket";
const std::string& user = "";
const std::string& password = "";
const std::string& precision = "ms";
const std::string& token = "E50icKBWaeccyZRwfdYDTtYxm10cHRPp8NRY0mp0upeEDZJC_STQfmJJzoBK9qCPm6mVUR9FhzysNlemzEmsOw==";

int main() {
// Create influx object
InfluxDatabase influx_db(host, port, org, bucket, user, password, precision, token);

// Check the health of the connection
influx_db.checkConnection(true);

EpitrendBinaryData binary_data;
std::ofstream epitrend_finished_file(Config::getOutputDir() + "times_inserted_into_SQL_server.txt", std::ios_base::app);
std::string times_finished_string;
for(int year = 2020; year < 2026; year++){
for(int month = 1; month < 13; month++) {
for(int day = 1; day < 32; day++) {
for(int hour = 0; hour < 25; hour++) {
    try {
        // Parse the Epitrend binary data file
        FileReader::parseEpitrendBinaryDataFile(binary_data,year,month,day,hour,false);

    } catch ( std::exception& e) {
        // Catching errors due to times that exist
        // std::cout << "No Epitrend data file found for: " << year << "," << month << "," << day << "," << hour << "\n" << e.what() << "\n";
        continue;
    }

    // Log the parsed file
    times_finished_string += std::to_string(year) + "," + std::to_string(month) + "," + std::to_string(day) + std::to_string(hour) + "\n";

    // Check the current size of the epitrend binary data object
    std::cout << "Current size of EpitrendBinaryData object: " << binary_data.getByteSize() << "\n";
    if (binary_data.getByteSize() > 0.1 * pow(10.0, 6.0) ) { // Limit INSERTS to 50 mb packs
        std::cout << "Curret epitrend data object exceeded size limit -> inserting data into SQL DB and flushing object...\n";
        
        // Insert data into SQL and flush current epitrend data
        binary_data.printFileAllTimeSeriesData("temp.txt");

        // ===================INFLUXDB VERSION===================
        // CHECK IF PART NAME IS IN NS TABLE
        // IF IT ISN'T
            // ADD ENTRY OF MACHINE NAME AND PART NAME INTO TABLE
            // GET NEW SENSOR_ID FOR NAME AND ALSO INTO TABLE
        // IF IT IS
            // GET THE SENSOR_ID
        // ENTER DATA INTO TS TABLE WITH ASSOCIATE SENSOR ID

        // Copy all data into influxDB table
        int num_tries_counter = 0;
        for (int i = 0; i < 3; ++i) {
            try {
                influx_db.copyEpitrendToBucket(binary_data, false);
                break; 
            } catch (std::exception& e) {
                std::cout << "Error in copying data to influxDB: " << e.what() << "\n Retrying...\n";
                num_tries_counter++;
                if (num_tries_counter == 3) {
                    std::cout << "Failed to copy data to influxDB after 3 tries\n";
                    return 0;
                }
            }
        }
        
        
        // Count the number of entries in the database
        int db_entry_count = 0;
        for(auto element : binary_data.getAllTimeSeriesData()){
            db_entry_count += element.second.size();
        }
        std::cout << "Number of entries in the database: " << db_entry_count << "\n";
        std::cout << "Approximate db size increased: " << db_entry_count * 100 << 
        " bytes" << " = " << db_entry_count * 100 / pow(10.0, 6.0) << " MB" << "\n";
        
        // Flush the current epitrend data object
        binary_data.clear();
        
        // Insert the completed times into log and flush
        epitrend_finished_file << times_finished_string;
        times_finished_string = "";
    }
}
}
}
}

return 0;
}