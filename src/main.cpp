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
        std::cout << time_now() << "Parsed " + GM + " Epitrend data file for: " << year << "," << month << "," << day << "," << hour << "\n";

    } catch ( std::exception& e) {
        // Catching errors due to times that exist
        std::cout << time_now() << "No " + GM + " Epitrend data file found for: " << year << "," << month << "," << day << "," << hour << "\n" << e.what() << "\n";
        return 0;
    }

    // Check the current size of the epitrend binary data object
    std::cout << time_now() << "Current size of " + GM + " EpitrendBinaryData object: " << binary_data.getByteSize() << "\n";
    if (binary_data.getByteSize() > 0.1 * pow(10.0, 6.0) ) { // Limit INSERTS to ~10 mb packs
        std::cout << time_now() << "Curret " + GM + " epitrend data object exceeded size limit -> inserting data into SQL DB and flushing object...\n";

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
                influx_db.copyEpitrendToBucket2(binary_data, false);
                break; 
            } catch (std::exception& e) {
                std::cout << time_now() << "Error in copying " + GM + " data to influxDB: " << e.what() << "\n Retrying...\n";
                num_tries_counter++;
                if (num_tries_counter == 100) {
                    std::cout << time_now() << "Failed to copy " + GM + " data to influxDB after 3 tries\n";
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
        std::cout << "Number of " + GM + " entries in the database: " << db_entry_count << "\n";
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
// // Create influx object
// InfluxDatabase influx_db(host, port, org, bucket, user, password, precision, token);

// // Check the health of the connection
// influx_db.checkConnection(true);

// EpitrendBinaryData binary_data_GM1, binary_data_GM2;
// for(int year = 2024; year < 3000; year++){
// for(int month = 12; month > 0; --month) {
// for(int day = 31; day > 1; --day) {
// for(int hour = 24; hour > -1; --hour) {
//     std::cout << time_now() << "Processing data for: " << year << "," << month << "," << day << "," << hour << "\n";
    
//     const auto copy_result_GM1 = copyEpitrendDataToInflux(influx_db, binary_data_GM1, "GM1", year, month, day, hour);
//     const auto copy_result_GM2 = copyEpitrendDataToInflux(influx_db, binary_data_GM2, "GM2", year, month, day, hour);
//     if (copy_result_GM1 < 0 || copy_result_GM2 < 0)
//     {
//         std::cout << time_now() << "Error in copying data to influxDB\n";
//         return -1;
//     }
// }
// }
// }
// }

//=====================START OF REAL-TIME DATA INSERTION=====================
const int sleep_seconds = 2;
const int max_reconnect_attempts = 100;

// Connect influxDB connection
InfluxDatabase influx_db(host, port, org, bucket, user, password, precision, token);

// Check the health of the connection
influx_db.checkConnection(true);

// Update the database real-time - every second
EpitrendBinaryData previous_binary_data_GM1, previous_binary_data_GM2,
current_binary_data_GM1, current_binary_data_GM2,
difference_binary_data_GM1, difference_binary_data_GM2;
while (true) {
std::cout << time_now() << "Updating database in real-time...\n";

// Grab the current year, month, day, and hour
auto now = std::chrono::system_clock::now();
std::time_t now_time = std::chrono::system_clock::to_time_t(now);
std::tm* now_tm = std::localtime(&now_time);
int year = now_tm->tm_year + 1900;
int month = now_tm->tm_mon + 1;
int day = now_tm->tm_mday;
int hour = now_tm->tm_hour;

std::cout << time_now() << "Processing data for: " << year << "," << month << "," << day << "," << hour << "\n";

// Load the epitrend binary data into binary object
try {
    FileReader::parseServerEpitrendBinaryDataFile(current_binary_data_GM1, "GM1", year, month, day, hour, false);
    FileReader::parseServerEpitrendBinaryDataFile(current_binary_data_GM2, "GM2", year, month, day, hour, false);
} catch (std::exception& e) {
    std::cout << time_now() << "No epitrend data file found for: " << year << "," << month << "," << day << "," << hour << "\n" << e.what() << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));
    continue;
}

// Find the difference between the previous and current data
difference_binary_data_GM1 = current_binary_data_GM1.difference(previous_binary_data_GM1);
difference_binary_data_GM2 = current_binary_data_GM2.difference(previous_binary_data_GM2);

// Copy the difference data to the influxDB
if(!difference_binary_data_GM1.is_empty()) {
    // Try to copy the data to influxDB with max_reconnect_attempts retries
    for(int i = 0; i < max_reconnect_attempts; ++i) {
        try {    
            std::cout << time_now() << "Found difference data for GM1... copying the following data into influxDB: \n";
            difference_binary_data_GM1.printAllTimeSeriesData();
            
            influx_db.copyEpitrendToBucket2(difference_binary_data_GM1, false);
            
            break;  
        
        } catch (std::exception& e) {
            std::cout << time_now() << "Error in copying GM1 data to influxDB: " << e.what() << "\n Retrying...\n";
            if (i == max_reconnect_attempts) {
                std::cout << time_now() << "Failed to copy GM1 data to influxDB after " << max_reconnect_attempts << " tries\n";
                return -1;
            }
                        
            // Pause
            std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

        }
    }
}
if(!difference_binary_data_GM2.is_empty()) {
    // Try to copy the data to influxDB with 100 retries
    for(int i = 0; i < max_reconnect_attempts; ++i) {
        try {    
            std::cout << time_now() << "Found difference data for GM2... copying the following data into influxDB: \n";
            difference_binary_data_GM2.printAllTimeSeriesData();

            influx_db.copyEpitrendToBucket2(difference_binary_data_GM2, false);
            
            break;
             
        } catch (std::exception& e) {
            std::cout << time_now() << "Error in copying GM2 data to influxDB: " << e.what() << "\n Retrying...\n";
            if (i == max_reconnect_attempts) {
                std::cout << time_now() << "Failed to copy GM2 data to influxDB after " << max_reconnect_attempts << " tries\n";
                return -1;
            }
                        
            // Pause
            std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

        }
    }
}

// Reset binary data
previous_binary_data_GM1 = current_binary_data_GM1;
previous_binary_data_GM2 = current_binary_data_GM2;

current_binary_data_GM1.clear();
current_binary_data_GM2.clear();

// Flush all data from epitrend binary data object if hour has changed
now = std::chrono::system_clock::now();
now_time = std::chrono::system_clock::to_time_t(now);
now_tm = std::localtime(&now_time);
if (now_tm->tm_hour != hour) {
    previous_binary_data_GM1.clear();
    previous_binary_data_GM2.clear();
}


std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

}

return 0;
}
