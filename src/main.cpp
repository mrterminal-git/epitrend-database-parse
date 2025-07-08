/**
 * @file main.cpp
 * @brief Entry point for the EPITREND GUI application.
 *
 * This program is designed to read binary data files from the Epitrend and RGA systems,
 * process the data, and insert it into an InfluxDB database. It supports both real-time
 * and historical data processing, leveraging multithreading to handle multiple tasks
 * concurrently.
 *
 * The program performs the following tasks:
 * - Real-time processing of RGA and Epitrend data.
 * - Historical processing of RGA and Epitrend data.
 * - Data insertion into InfluxDB with retry mechanisms for robustness.
 * - Logging of operations and database statistics.
 *
 * Dependencies:
 * - Common.hpp: Contains common utility functions and definitions.
 * - FileReader.hpp: Provides functions to read and parse binary data files.
 * - Config.hpp: Manages configuration settings for the program.
 * - EpitrendBinaryFormat.hpp: Defines the format of the Epitrend binary data.
 * - EpitrendBinaryData.hpp: Represents the parsed binary data.
 * - AzureDatabase.hpp: Provides functions to interact with an Azure SQL database.
 * - InfluxDatabase.hpp: Provides functions to interact with an InfluxDB database.
 * - RGAData.hpp: Represents the parsed RGA data.
 * - curl/curl.h: Used for making HTTP requests to the InfluxDB server.
 */

#include "Common.hpp"
#include "FileReader.hpp"
#include "Config.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"
#include "AzureDatabase.hpp"
#include "InfluxDatabase.hpp"
#include "influxdb.hpp"
#include "RGAData.hpp"
#include <curl/curl.h>
#include <future>

// Global config
const Config config("config.txt");

// Define the constants
const std::string& org = config.getOrg();
const std::string& host = config.getHost();
const int port = config.getPort();
const std::string& rga_bucket = config.getRgaBucket();
const std::string& epitrend_bucket = config.getEpitrendBucket();
const std::string& user = config.getUser();
const std::string& password = config.getPassword();
const std::string& precision = config.getPrecision();
const std::string& token = config.getToken();

/**
 * @brief Retrieves the current year as an integer.
 *
 * This function uses the `<chrono>` library to get the current system time and extracts
 * the year from it. The year is calculated based on the `tm_year` field of the `std::tm`
 * structure, which represents the number of years since 1900.
 *
 * @return An integer representing the current year (e.g., 2025).
 *
 * Example Usage:
 * @code
 * int currentYear = getCurrentYear();
 * std::cout << "Current Year: " << currentYear << std::endl;
 * @endcode
 *
 * Dependencies:
 * - `<chrono>`: Used to retrieve the current system time.
 * - `<ctime>`: Used to convert the system time to a `std::tm` structure.
 *
 * @throws None
 */
int getCurrentYear() {
    // Get the current time
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    // Convert to local time
    std::tm* now_tm = std::localtime(&now_time);

    // Extract the year and return it
    return now_tm->tm_year + 1900; // tm_year is years since 1900
}

/**
 * @brief Retrieves the current timestamp as a formatted string.
 * @return A string representing the current time in the format "YYYY-MM-DD HH:MM:SS||".
 */
std::string time_now() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::string time_str = std::ctime(&now_time);
    time_str.pop_back();
    return time_str + "|| ";
}

/**
 * @brief Copies Epitrend binary data to the InfluxDB database.
 * @param influx_db The InfluxDB object for database interaction.
 * @param binary_data The EpitrendBinaryData object containing parsed data.
 * @param GM The metadata identifier (e.g., "GM1").
 * @param year The year of the data file.
 * @param month The month of the data file.
 * @param day The day of the data file.
 * @param hour The hour of the data file.
 * @return 1 if successful, 0 if no data file is found, -1 if an error occurs.
 */
int copyEpitrendDataToInflux(InfluxDatabase influx_db, 
EpitrendBinaryData& binary_data, 
std::string GM, 
int year, 
int month, 
int day, 
int hour) {
    try {
        // Parse the Epitrend binary data file
        FileReader::parseServerEpitrendBinaryDataFile(config, binary_data, GM, year,month,day,hour,false);
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

/**
 * @brief Copies RGA data to the InfluxDB database.
 * @param influx_db The InfluxDB object for database interaction.
 * @param rga_data The RGAData object containing parsed data.
 * @param GM The metadata identifier (e.g., "GM1").
 * @param year The year of the data file.
 * @param month The month of the data file.
 * @param day The day of the data file.
 * @return 1 if successful, 0 if no data file is found, -1 if an error occurs.
 */
int copyRGADataToInflux(InfluxDatabase influx_db,
RGAData& rga_data,
std::string GM,
int year,
int month,
int day) {

try {
    // Parse the RGA data file
    FileReader::parseServerRGADataFile(config, rga_data, GM, year, month, day, false);
    std::cout << time_now() << "Parsed " + GM + " RGA data file for: " << year << "," << month << "," << day << "\n";

} catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    std::cout << time_now() << "No " + GM + " RGA data file found for: " << year << "," << month << "," << day << "\n" << e.what() << "\n";

}

// Check the current size of the RGA binary data object
std::cout << time_now() << "Currently copying " + GM + " RGA data object into DB for " << year << "," << month << "," << day << "\n";
std::cout << time_now() << "Current size of " + GM + " RGAData object: " << rga_data.getByteSize() << "\n";
if (rga_data.getByteSize() > 0.1 * pow(10.0, 6.0) ) { // Limit INSERTS to ~10 mb packs
    std::cout << time_now() << "Curret " + GM + " RGA data object exceeded size limit -> inserting data into SQL DB and flushing object...\n";

    int num_tries_counter = 0;
    for (int i = 0; i < 100; ++i) {
        try {
            influx_db.copyRGADataToBucket(rga_data, false);
            break;
        } catch (const std::exception& e) {
            std::cout << time_now() << "Error in copying " + GM + " RGA data to influxDB: " << e.what() << "\n Retrying...\n";
            std::cerr << e.what() << "\n";
            if (num_tries_counter == 100) {
                std::cout << time_now() << "Failed to copy " + GM + " RGA data to influxDB after 100 tries\n";
                return -1;

            }

        }
    }

    // Count the number of entries in the database
    int db_entry_count = 0;
    for(auto element : rga_data.getAllTimeSeriesData()){
        db_entry_count += element.second.size();
    }
    std::cout << "Number of " + GM + " RGA entries in the database: " << db_entry_count << "\n";
    std::cout << "Approximate db size increased: " << db_entry_count * 100 << 
    " bytes" << " = " << db_entry_count * 100 / pow(10.0, 6.0) << " MB" << "\n";
    
    // Flush the current RGA data object
    rga_data.clearData();
}    

return 1;
}

/**
 * @brief Processes real-time RGA data and inserts it into the InfluxDB database.
 * @param exitSignal A promise object to signal thread completion.
 * @details Continuously monitors and updates the database with real-time RGA data.
 * Implements retry mechanisms for database operations.
 */
void processRealTimeRGAData(std::promise<void> exitSignal) {
    try{
        const int& parse_error_sleep_seconds = 1;
        const int& sleep_seconds = 2;  //60 seconds * 60 minutes = 1 hour
        const int& max_reconnect_attempts = 100;
        const int& integration_count = 4;

        // Connect influxDB connection
        InfluxDatabase influx_db(host, port, org, rga_bucket, user, password, precision, token);

        // Check the health of the connection
        influx_db.checkConnection(true);

        // Update the database real-time - every second
        RGAData previous_RGA_data_GM1(integration_count), previous_RGA_data_GM2(integration_count),
        previous_RGA_data_Cluster(integration_count), previous_RGA_data_PM1(integration_count),
        previous_RGA_data_PM2(integration_count),
        current_RGA_data_GM1(integration_count), current_RGA_data_GM2(integration_count),
        current_RGA_data_Cluster(integration_count), current_RGA_data_PM1(integration_count),
        current_RGA_data_PM2(integration_count),
        difference_RGA_data_GM1, difference_RGA_data_GM2,
        difference_RGA_data_Cluster, difference_RGA_data_PM1,
        difference_RGA_data_PM2;

        while (true) {
        std::cout << time_now() << "processRealTimeRGAData||" << "Updating database in real-time...\n";

        // Grab the current year, month, day, and hour
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_time);
        int year = now_tm->tm_year + 1900;
        int month = now_tm->tm_mon + 1;
        int day = now_tm->tm_mday - 1;
        int hour = now_tm->tm_hour;

        std::cout << time_now() << "processRealTimeRGAData||" << "Processing RGA data for: " << year << "," << month << "," << day << "\n";

        // Load the entire week's RGA data into RGAData object
        for(int loop_day = day; loop_day > day - 7; --loop_day) {
            int loop_month = month;
            int loop_year = year;
            // Update month if day is less than 1
            if (loop_day < 1) {
                loop_day = 31;
                loop_month -= 1;
                if (month < 1) {
                    loop_month = 12;
                    loop_year -= 1;
                }
            }

            // Parse GM1 RGA Data
            try {
                FileReader::parseServerRGADataFile(config, current_RGA_data_GM1, "GM1", loop_year, loop_month, loop_day, false);
                std::cout << time_now() << "processRealTimeRGAData||" << "Parsed GM1 RGA data file for: " << loop_year << "," << loop_month << "," << loop_day << "\n";
            } catch (std::exception& e) {
                std::cout << time_now() << "processRealTimeRGAData||" << "Warning during parsing GM1 RGA data file for " << loop_year << "," << loop_month << "," << loop_day << ": " << e.what() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(parse_error_sleep_seconds));
            }

            // Parse GM2 RGA Data
            try {
                FileReader::parseServerRGADataFile(config, current_RGA_data_GM2, "GM2", loop_year, loop_month, loop_day, false);
                std::cout << time_now() << "processRealTimeRGAData||" << "Parsed GM2 RGA data file for: " << loop_year << "," << loop_month << "," << loop_day << "\n";
            } catch (std::exception& e) {
                std::cout << time_now() << "processRealTimeRGAData||" << "Warning during parsing GM2 RGA data file for " << loop_year << "," << loop_month << "," << loop_day << ": " << e.what() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(parse_error_sleep_seconds));
            }

            // Parse Cluster RGA Data
            try {
                FileReader::parseServerRGADataFile(config, current_RGA_data_Cluster, "Cluster", loop_year, loop_month, loop_day, false);
                std::cout << time_now() << "processRealTimeRGAData||" << "Parsed Cluster RGA data file for: " << loop_year << "," << loop_month << "," << loop_day << "\n";
            } catch (std::exception& e) {
                std::cout << time_now() << "processRealTimeRGAData||" << "Warning during parsing Cluster RGA data file for " << loop_year << "," << loop_month << "," << loop_day << ": " << e.what() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(parse_error_sleep_seconds));
            }

            // Parse PM1 RGA Data
            try {
                FileReader::parseServerRGADataFile(config, current_RGA_data_PM1, "PM1", loop_year, loop_month, loop_day, false);
                std::cout << time_now() << "processRealTimeRGAData||" << "Parsed PM1 RGA data file for: " << loop_year << "," << loop_month << "," << loop_day << "\n";
            } catch (std::exception& e) {
                std::cout << time_now() << "processRealTimeRGAData||" << "Warning during parsing PM1 RGA data file for " << loop_year << "," << loop_month << "," << loop_day << ": " << e.what() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(parse_error_sleep_seconds));
            }

            // Parse PM2 RGA Data
            try {
                FileReader::parseServerRGADataFile(config, current_RGA_data_PM2, "PM2", loop_year, loop_month, loop_day, false);
                std::cout << time_now() << "processRealTimeRGAData||" << "Parsed PM2 RGA data file for: " << loop_year << "," << loop_month << "," << loop_day << "\n";
            } catch (std::exception& e) {
                std::cout << time_now() << "processRealTimeRGAData||" << "Warning during parsing PM2 RGA data file for " << loop_year << "," << loop_month << "," << loop_day << ": " << e.what() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(parse_error_sleep_seconds));
            }
        }

        // Find the difference between the previous and current data
        difference_RGA_data_GM1 = current_RGA_data_GM1.difference(previous_RGA_data_GM1);
        difference_RGA_data_GM2 = current_RGA_data_GM2.difference(previous_RGA_data_GM2);
        difference_RGA_data_Cluster = current_RGA_data_Cluster.difference(previous_RGA_data_Cluster);
        difference_RGA_data_PM1 = current_RGA_data_GM2.difference(previous_RGA_data_PM1);
        difference_RGA_data_PM2 = current_RGA_data_GM2.difference(previous_RGA_data_PM2);

        // Copy the difference data to the influxDB
        if(!difference_RGA_data_GM1.is_empty()) {
            // Try to copy the data to influxDB with max_reconnect_attempts retries
            for(int i = 0; i < max_reconnect_attempts; ++i) {
                try {    
                    std::cout << time_now() << "processRealTimeRGAData||" << "Found difference data for GM1... copying the following data into influxDB: \n";            
                    influx_db.copyRGADataToBucket(difference_RGA_data_GM1, false);
                    
                    break;  
                
                } catch (std::exception& e) {
                    std::cout << time_now() << "processRealTimeRGAData||" << "Error in copying GM1 RGA data to influxDB: " << e.what() << "\n Retrying...\n";
                    if (i == max_reconnect_attempts) {
                        std::cout << time_now() << "processRealTimeRGAData||" << "Failed to copy GM1 RGA data to influxDB after " << max_reconnect_attempts << " tries\n";
                        exit(-1);
                    }
                                
                    // Pause
                    std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

                }
            }
        }
        else {
            std::cout << time_now() << "processRealTimeRGAData||" << "No difference data found for GM1\n";
        }
        if(!difference_RGA_data_GM2.is_empty()) {
                // Try to copy the data to influxDB with 100 retries
            for(int i = 0; i < max_reconnect_attempts; ++i) {
                try {    
                    std::cout << time_now() << "processRealTimeRGAData||" << "Found difference data for GM2... copying the following data into influxDB: \n";
                    influx_db.copyRGADataToBucket(difference_RGA_data_GM2, false);
                    
                    break;
                    
                } catch (std::exception& e) {
                    std::cout << time_now() << "processRealTimeRGAData||" << "Error in copying GM2 data to influxDB: " << e.what() << "\n Retrying...\n";
                    if (i == max_reconnect_attempts) {
                        std::cout << time_now() << "processRealTimeRGAData||" << "Failed to copy GM2 data to influxDB after " << max_reconnect_attempts << " tries\n";
                        exit(-1);
                    }
                                
                    // Pause
                    std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

                }
            }
        } else {
            std::cout << time_now() << "processRealTimeRGAData||" << "No difference data found for GM2\n";
        }
        if(!difference_RGA_data_Cluster.is_empty()) {
            // Try to copy the data to influxDB with 100 retries
            for(int i = 0; i < max_reconnect_attempts; ++i) {
                try {    
                    std::cout << time_now() << "processRealTimeRGAData||" << "Found difference data for Cluster... copying the following data into influxDB: \n";
                    influx_db.copyRGADataToBucket(difference_RGA_data_Cluster, false);
                    
                    break;
                    
                } catch (std::exception& e) {
                    std::cout << time_now() << "processRealTimeRGAData||" << "Error in copying Cluster data to influxDB: " << e.what() << "\n Retrying...\n";
                    if (i == max_reconnect_attempts) {
                        std::cout << time_now() << "processRealTimeRGAData||" << "Failed to copy Cluster data to influxDB after " << max_reconnect_attempts << " tries\n";
                        exit(-1);
                    }
                                
                    // Pause
                    std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

                }
            }
        } else {
            std::cout << time_now() << "processRealTimeRGAData||" << "No difference data found for Cluster\n";
        }
        if(!difference_RGA_data_PM1.is_empty()) {
            // Try to copy the data to influxDB with max_reconnect_attempts retries
            for(int i = 0; i < max_reconnect_attempts; ++i) {
                try {    
                    std::cout << time_now() << "processRealTimeRGAData||" << "Found difference data for PM1... copying the following data into influxDB: \n";            
                    influx_db.copyRGADataToBucket(difference_RGA_data_PM1, false);
                    
                    break;  
                
                } catch (std::exception& e) {
                    std::cout << time_now() << "processRealTimeRGAData||" << "Error in copying PM1 RGA data to influxDB: " << e.what() << "\n Retrying...\n";
                    if (i == max_reconnect_attempts) {
                        std::cout << time_now() << "processRealTimeRGAData||" << "Failed to copy PM1 RGA data to influxDB after " << max_reconnect_attempts << " tries\n";
                        exit(-1);
                    }
                                
                    // Pause
                    std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

                }
            }
        }
        else {
            std::cout << time_now() << "processRealTimeRGAData||" << "No difference data found for PM1\n";
        }
        if(!difference_RGA_data_PM2.is_empty()) {
            // Try to copy the data to influxDB with max_reconnect_attempts retries
            for(int i = 0; i < max_reconnect_attempts; ++i) {
                try {    
                    std::cout << time_now() << "processRealTimeRGAData||" << "Found difference data for PM2... copying the following data into influxDB: \n";            
                    influx_db.copyRGADataToBucket(difference_RGA_data_PM2, false);
                    
                    break;  
                
                } catch (std::exception& e) {
                    std::cout << time_now() << "processRealTimeRGAData||" << "Error in copying PM2 RGA data to influxDB: " << e.what() << "\n Retrying...\n";
                    if (i == max_reconnect_attempts) {
                        std::cout << time_now() << "processRealTimeRGAData||" << "Failed to copy PM2 RGA data to influxDB after " << max_reconnect_attempts << " tries\n";
                        exit(-1);
                    }
                                
                    // Pause
                    std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

                }
            }
        }
        else {
            std::cout << time_now() << "processRealTimeRGAData||" << "No difference data found for PM2\n";
        }

        // Reset RGA data
        previous_RGA_data_GM1 = current_RGA_data_GM1;
        previous_RGA_data_GM2 = current_RGA_data_GM2;
        previous_RGA_data_Cluster = current_RGA_data_Cluster;
        previous_RGA_data_PM1 = current_RGA_data_PM1;
        previous_RGA_data_PM2 = current_RGA_data_PM2;

        current_RGA_data_GM1.clearData();
        current_RGA_data_GM2.clearData();
        current_RGA_data_Cluster.clearData();
        current_RGA_data_PM1.clearData();
        current_RGA_data_PM2.clearData();

        // Flush all data from RGA data object if hour has changed
        now = std::chrono::system_clock::now();
        now_time = std::chrono::system_clock::to_time_t(now);
        now_tm = std::localtime(&now_time);
        if (now_tm->tm_hour != hour) {
            previous_RGA_data_GM1.clearData();
            previous_RGA_data_GM2.clearData();
            previous_RGA_data_Cluster.clearData();
            previous_RGA_data_PM1.clearData();
            previous_RGA_data_PM2.clearData();
        }

        std::cout << time_now() << "processRealTimeRGAData||" << "Sleeping for " << sleep_seconds << " seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

        }

        exitSignal.set_value();

    } catch (const std::exception& e) {
        std::cerr << "Exception in processRealTimeRGAData: " << e.what() << std::endl;
        exitSignal.set_exception(std::current_exception());

    } catch (...) {
        std::cerr << "Unknown exception in processRealTimeRGAData" << std::endl;
        exitSignal.set_exception(std::current_exception());

    }
}

/**
 * @brief Processes historical RGA data and inserts it into the InfluxDB database.
 * @param exitSignal A promise object to signal thread completion.
 * @details Iterates over historical RGA data files and inserts them into the database.
 */
void processHistoricalRGAData(std::promise<void> exitSignal) {  
    try{  
        // Create influx object
        InfluxDatabase influx_db(host, port, org, rga_bucket, user, password, precision, token);

        // Check the health of the connection
        influx_db.checkConnection(true);

        // Set integration limits and construct RGAData objects (e.g. integration_count = 4 => {+/-0.4}*Integer)
        const int& integration_count = 4;
        RGAData GM1_rga_data(integration_count), 
        GM2_rga_data(integration_count),
        Cluster_rga_data(integration_count),
        PM1_rga_data(integration_count), 
        PM2_rga_data(integration_count);

        int current_year = getCurrentYear();
        for(int year = current_year; year > 2020; --year){
        for(int month = 12; month > 0; --month) {
        for(int day = 31; day > 0; --day) {
            const auto copy_result_GM1 = copyRGADataToInflux(influx_db, GM1_rga_data, "GM1", year, month, day);
            const auto copy_result_GM2 = copyRGADataToInflux(influx_db, GM2_rga_data, "GM2", year, month, day);
            const auto copy_result_Cluster = copyRGADataToInflux(influx_db, Cluster_rga_data, "Cluster", year, month, day);
            const auto copy_result_PM1 = copyRGADataToInflux(influx_db, PM1_rga_data, "PM1", year, month, day);
            const auto copy_result_PM2 = copyRGADataToInflux(influx_db, PM2_rga_data, "PM2", year, month, day);
            if (copy_result_GM1 < 0 || copy_result_GM2 < 0 || copy_result_Cluster < 0 || copy_result_PM1 < 0 || copy_result_PM2 < 0)
            {
                std::cout << time_now() << "processHistoricalRGAData|| " << "Error in copying data to influxDB\n";
                exit(-1);
            }
            std::cout << "--------------------------------------------\n";
        }
        }
        }
        
        exitSignal.set_value();

    } catch (const std::exception& e) {
        std::cerr << "Exception in processHistoricalRGAData: " << e.what() << std::endl;
        exitSignal.set_exception(std::current_exception());

    } catch (...) {
        std::cerr << "Unknown exception in processHistoricalRGAData" << std::endl;
        exitSignal.set_exception(std::current_exception());

    }
}

/**
 * @brief Processes historical Epitrend data and inserts it into the InfluxDB database.
 * @param exitSignal A promise object to signal thread completion.
 * @details Iterates over historical Epitrend data files and inserts them into the database.
 */
void processHistoricalEpitrendData(std::promise<void> exitSignal) {
    try {
        // Create influx object
        InfluxDatabase influx_db(host, port, org, epitrend_bucket, user, password, precision, token);

        // Check the health of the connection
        influx_db.checkConnection(true);

        int current_year = getCurrentYear();
        EpitrendBinaryData binary_data_GM1, binary_data_GM2;
        for(int year = current_year; year > 2019; --year){
        for(int month = 12; month > 0; --month) {
        for(int day = 31; day > 1; --day) {
        for(int hour = 24; hour > -1; --hour) {
            std::cout << time_now() << "Processing data for: " << year << "," << month << "," << day << "," << hour << "\n";
            
            const auto copy_result_GM1 = copyEpitrendDataToInflux(influx_db, binary_data_GM1, "GM1", year, month, day, hour);
            const auto copy_result_GM2 = copyEpitrendDataToInflux(influx_db, binary_data_GM2, "GM2", year, month, day, hour);
            if (copy_result_GM1 < 0 || copy_result_GM2 < 0)
            {
                std::cout << time_now() << "Error in copying data to influxDB\n";
                exit(-1);
            }
        }
        }
        }
        }
        
        exitSignal.set_value();

    } catch (const std::exception& e) {
        std::cerr << "Exception in processHistoricalEpitrendData: " << e.what() << std::endl;
        exitSignal.set_exception(std::current_exception());

    } catch (...) {
        std::cerr << "Unknown exception in processHistoricalEpitrendData" << std::endl;
        exitSignal.set_exception(std::current_exception());

    }

}

/**
 * @brief Processes real-time Epitrend data and inserts it into the InfluxDB database.
 * @param exitSignal A promise object to signal thread completion.
 * @details Continuously monitors and updates the database with real-time Epitrend data.
 * Implements retry mechanisms for database operations.
 */
void processRealTimeEpitrendData(std::promise<void> exitSignal) {
    const int& sleep_seconds = 10;
    const int& max_reconnect_attempts = 100;

    try {
        // Connect influxDB connection
        InfluxDatabase influx_db(host, port, org, epitrend_bucket, user, password, precision, token);

        // Check the health of the connection
        influx_db.checkConnection(true);

        // Update the database real-time - every second
        EpitrendBinaryData previous_binary_data_GM1, previous_binary_data_GM2,
        current_binary_data_GM1, current_binary_data_GM2,
        difference_binary_data_GM1, difference_binary_data_GM2;

        while (true) {
        std::cout << time_now() << "processRealTimeEpitrendData|| " << "Updating database in real-time...\n";

        // Grab the current year, month, day, and hour
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_time);
        int year = now_tm->tm_year + 1900;
        int month = now_tm->tm_mon + 1;
        int day = now_tm->tm_mday;
        int hour = now_tm->tm_hour;

        std::cout << time_now() << "processRealTimeEpitrendData|| " << "Processing data for: " << year << "," << month << "," << day << "," << hour << "\n";

        // Load the epitrend binary data into binary object
        try {
            FileReader::parseServerEpitrendBinaryDataFile(config, current_binary_data_GM1, "GM1", year, month, day, hour, false);
            FileReader::parseServerEpitrendBinaryDataFile(config, current_binary_data_GM2, "GM2", year, month, day, hour, false);
        } catch (std::exception& e) {
            std::cout << time_now() << "processRealTimeEpitrendData|| " << "No epitrend data file found for: " << year << "," << month << "," << day << "," << hour << "\n" << e.what() << "\n";
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
                    std::cout << time_now() << "processRealTimeEpitrendData|| " << "Found difference data for GM1... copying the following data into influxDB: \n";
                    // difference_binary_data_GM1.printAllTimeSeriesData();
                    
                    influx_db.copyEpitrendToBucket2(difference_binary_data_GM1, false);
                    
                    break;  
                
                } catch (std::exception& e) {
                    std::cout << time_now() << "processRealTimeEpitrendData|| " << "Error in copying GM1 data to influxDB: " << e.what() << "\n Retrying...\n";
                    if (i == max_reconnect_attempts) {
                        std::cout << time_now() << "processRealTimeEpitrendData|| " << "Failed to copy GM1 data to influxDB after " << max_reconnect_attempts << " tries\n";
                        exit(-1);
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
                    std::cout << time_now() << "processRealTimeEpitrendData|| " << "Found difference data for GM2... copying the following data into influxDB: \n";
                    // difference_binary_data_GM2.printAllTimeSeriesData();

                    influx_db.copyEpitrendToBucket2(difference_binary_data_GM2, false);
                    
                    break;
                    
                } catch (std::exception& e) {
                    std::cout << time_now() << "processRealTimeEpitrendData|| " << "Error in copying GM2 data to influxDB: " << e.what() << "\n Retrying...\n";
                    if (i == max_reconnect_attempts) {
                        std::cout << time_now() << "processRealTimeEpitrendData|| " << "Failed to copy GM2 data to influxDB after " << max_reconnect_attempts << " tries\n";
                        exit(-1);
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

        exitSignal.set_value();

    } catch (const std::exception& e) {
        std::cerr << "Exception in processRealTimeEpitrendData: " << e.what() << std::endl;
        exitSignal.set_exception(std::current_exception());
    
    } catch (...) {
        std::cerr << "Unknown exception in processRealTimeEpitrendData" << std::endl;
        exitSignal.set_exception(std::current_exception());

    }
}

/**
 * @brief Entry point for the EPITREND GUI application.
 * @return 0 on successful execution, -1 on error.
 * @details Initializes threads for real-time and historical data processing for both
 * Epitrend and RGA systems. Monitors thread execution and handles exceptions.
 */
int main() {
    std::cout << "org: " << org << "\n";
    std::cout << "host: " << host << "\n";
    std::cout << "port: " << port << "\n";
    std::cout << "rga_bucket: " << rga_bucket << "\n";
    std::cout << "epitrend_bucket: " << epitrend_bucket << "\n";
    std::cout << "user: " << user << "\n";
    std::cout << "password: " << password << "\n";
    std::cout << "precision: " << precision << "\n";
    std::cout << "token: " << token << "\n";

    // Create promises and futures for each thread
    std::promise<void> promiseRealTimeRGA, promiseHistoricalRGA, promiseHistoricalEpitrend, promiseRealTimeEpitrend;
    std::future<void> futureRealTimeRGA = promiseRealTimeRGA.get_future();
    std::future<void> futureHistoricalRGA = promiseHistoricalRGA.get_future();
    std::future<void> futureHistoricalEpitrend = promiseHistoricalEpitrend.get_future();
    std::future<void> futureRealTimeEpitrend = promiseRealTimeEpitrend.get_future();

    // Create and start threads
    std::thread threadRealTimeRGADataToInflux(processRealTimeRGAData, std::move(promiseRealTimeRGA));
    std::thread threadHistoricalRGADataToInflux(processHistoricalRGAData, std::move(promiseHistoricalRGA));
    std::thread threadHistoricalEpitrendDataToInflux(processHistoricalEpitrendData, std::move(promiseHistoricalEpitrend));
    std::thread threadRealTimeEpitrendDataToInflux(processRealTimeEpitrendData, std::move(promiseRealTimeEpitrend));

    // Monitor the futures to detect when threads have stopped running
    std::vector<std::future<void>*> futures = {&futureRealTimeRGA, &futureHistoricalRGA, &futureHistoricalEpitrend, &futureRealTimeEpitrend};
    while (!futures.empty()) {
        for (auto it = futures.begin(); it != futures.end();) {
            std::future_status status = (*it)->wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::ready) {
                try {
                    (*it)->get(); // Check for exceptions
                    std::cout << "Thread completed successfully.\n";
                } catch (const std::exception& e) {
                    std::cerr << "Thread exited with exception: " << e.what() << std::endl;
                    exit(-1);
                }
                it = futures.erase(it); // Remove the future from the list
            } else {
                ++it;
            }
        }
    }

    // Join all threads
    threadRealTimeRGADataToInflux.join();
    threadHistoricalRGADataToInflux.join();
    threadHistoricalEpitrendDataToInflux.join();
    threadRealTimeEpitrendDataToInflux.join();


return 0;
}
