#ifndef EPITRENDBINARYDATA_HPP
#define EPITRENDBINARYDATA_HPP

#include "Common.hpp"

/**
 * @class EpitrendBinaryData
 * @brief A class for managing and processing time-series data from Epitrend binary files.
 *
 * The `EpitrendBinaryData` class provides functionality to store, manipulate, and retrieve
 * time-series data. It supports operations such as adding data items, printing data, clearing
 * data, and calculating differences between two datasets.
 */
class EpitrendBinaryData {
public:
    // Constructors
    /**
     * @brief Default constructor for the EpitrendBinaryData class.
     */
    EpitrendBinaryData() = default;


    // Setters
    /**
     * @brief Adds a time-series data item to the object.
     * @param name The name of the time-series data (e.g., sensor name).
     * @param time_series A pair containing the timestamp and value.
     * @param verbose If true, prints a warning when overwriting existing data.
     */
    void addDataItem(
        std::string name, 
        std::pair<double,double> time_series, 
        bool verbose = false
    );


    // Getters
    /**
     * @brief Retrieves all time-series data stored in the object.
     * @return A map containing all time-series data, where the key is the name and the value is a map of timestamps and values.
     */
    std::unordered_map<std::string, std::unordered_map<double,double>> getAllTimeSeriesData();

    /**
     * @brief Retrieves the total byte size of the object.
     * @return The byte size of the object, including all stored data.
     */
    int getByteSize();

    // Utility Methods
    /**
     * @brief Prints all time-series data to the console.
     */
    void printAllTimeSeriesData();

    /**
     * @brief Writes all time-series data to a file.
     * @param config The configuration object containing the output directory.
     * @param filename The name of the file to write the data to.
     */
    void printFileAllTimeSeriesData(const Config& config, const std::string& filename);

    /**
     * @brief Checks if the object contains any time-series data.
     * @return True if the object is empty, false otherwise.
     */
    bool is_empty();

    /**
     * @brief Clears all time-series data from the object.
     */
    void clear();

    /**
     * @brief Calculates the difference between two EpitrendBinaryData objects.
     * @param other The other EpitrendBinaryData object to compare against.
     * @return A new EpitrendBinaryData object containing the difference.
     */
    EpitrendBinaryData difference(EpitrendBinaryData& other) const;


private:
    /**
     * @brief A map to store all time-series data.
     * The key is the name of the data (e.g., sensor name), and the value is a map of timestamps and values.
     */
    std::unordered_map<std::string, std::unordered_map<double,double>> allTimeSeriesData;

    /**
     * @brief The total byte size of the object, including all stored data.
     */
    int byteSize = 0;
};

#endif // EPITRENDBINARYDATA_HPP
