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

std::string time_now() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::string time_str = std::ctime(&now_time);
    time_str.pop_back();
    return time_str + "|| ";
}

int copyEpitrendDataToInflux(InfluxDatabase influx_db, 
EpitrendBinaryData& binary_data, 
std::string GM, 
int year, 
int month, 
int day, 
int hour) {
    try {
        // Parse the Epitrend binary data file
        FileReader::parseEpitrendBinaryDataFile(binary_data, GM, year,month,day,hour,false);
        std::cout << time_now() << "Parsed Epitrend data file for: " << year << "," << month << "," << day << "," << hour << "\n";

    } catch ( std::exception& e) {
        // Catching errors due to times that exist
        // std::cout << "No Epitrend data file found for: " << year << "," << month << "," << day << "," << hour << "\n" << e.what() << "\n";
        return 0;
    }

    // Check the current size of the epitrend binary data object
    std::cout << time_now() << "Current size of EpitrendBinaryData object: " << binary_data.getByteSize() << "\n";
    if (binary_data.getByteSize() > 0.1 * pow(10.0, 6.0) ) { // Limit INSERTS to ~10 mb packs
        std::cout << time_now() << "Curret epitrend data object exceeded size limit -> inserting data into SQL DB and flushing object...\n";

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
        for (int i = 0; i < 100; ++i) {
            try {
                influx_db.copyEpitrendToBucket(binary_data, false);
                break; 
            } catch (std::exception& e) {
                std::cout << time_now() << "Error in copying data to influxDB: " << e.what() << "\n Retrying...\n";
                num_tries_counter++;
                if (num_tries_counter == 100) {
                    std::cout << time_now() << "Failed to copy data to influxDB after 3 tries\n";
                    return -1;
                }

                // Pause for 10 seconds
                std::this_thread::sleep_for(std::chrono::seconds(10));

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
    }
    return 1;
}

// Define the constants
const std::string& org = "au-mbe-eng";
const std::string& host = "127.0.0.1";
const int port = 8086;
const std::string& bucket = "MBE_BMS";
const std::string& user = "";
const std::string& password = "";
const std::string& precision = "ms";
const std::string& token = "142ce8c4d871f807e6f8c3c264afcb5588d7c82ecaad305d8fde09f3f5dec642";

int main() {
// Create influx object
InfluxDatabase influx_db(host, port, org, bucket, user, password, precision, token);

// Check the health of the connection
influx_db.checkConnection(true);

EpitrendBinaryData binary_data;
for(int year = 2020; year < 3000; year++){
for(int month = 12; month > 0; --month) {
for(int day = 31; day > 1; --day) {
for(int hour = 24; hour > -1; --hour) {
    std::cout << time_now() << "Processing data for: " << year << "," << month << "," << day << "," << hour << "\n";
    
    const auto copy_result_GM1 = copyEpitrendDataToInflux(influx_db, binary_data, "GM1", year, month, day, hour);
    const auto copy_result_GM2 = copyEpitrendDataToInflux(influx_db, binary_data, "GM2", year, month, day, hour);
    if (copy_result_GM1 < 0 || copy_result_GM2 < 0)
    {
        std::cout << time_now() << "Error in copying data to influxDB\n";
        return -1;
    } else if (copy_result_GM1 == 0 || copy_result_GM2 == 0) {
        std::cout << time_now() << "No Epitrend data file found for: " << year << "," << month << "," << day << "," << hour << "\n";
    }
}
}
}
}

return 0;
}