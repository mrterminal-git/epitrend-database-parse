#ifndef EPITRENDBINARYFORMAT_HPP
#define EPITRENDBINARYFORMAT_HPP

#include "Common.hpp"

/**
 * @class EpitrendBinaryFormat
 * @brief A class for managing and processing metadata from Epitrend binary format files.
 *
 * The `EpitrendBinaryFormat` class provides functionality to store, manipulate, and retrieve
 * metadata about EpiTrend time-series data files. It supports operations such as adding data items,
 * retrieving metadata, clearing stored data, and printing summaries.
 */
class EpitrendBinaryFormat {
public:
    /**
     * @struct DataItem
     * @brief Represents metadata for a single data item in the Epitrend binary format.
     *
     * The `DataItem` struct contains information about a specific data item, including its name,
     * type, range, total values, and value offset.
     */
    struct DataItem {
        std::string Name; ///< The name of the data item.
        std::string Type; ///< The type of the data item (e.g., "float", "int").
        std::string Range; ///< The range of values for the data item.
        int TotalValues = 0; ///< The total number of values for the data item.
        int ValueOffset = 0; ///< The offset of the data item in the binary file.

        /**
         * @brief Overloads the `<<` operator to print the `DataItem` metadata.
         * @param os The output stream.
         * @param item The `DataItem` object to print.
         * @return The output stream with the formatted `DataItem` metadata.
         */
        friend std::ostream& operator<<(std::ostream& os, const DataItem& item) {
            os << "Name: " << item.Name << ", Type: " << item.Type 
               << ", Range: " << item.Range << ", TotalValues: " 
               << item.TotalValues << ", ValueOffset: " << item.ValueOffset;
            return os;
        }
    };

    // Constructors
    /**
     * @brief Default constructor for the `EpitrendBinaryFormat` class.
     */
    EpitrendBinaryFormat() = default;


    // Setters
    /**
     * @brief Sets the current day for the binary format.
     * @param currentDay The current day value.
     */
    void setCurrentDay(int currentDay);

    /**
     * @brief Sets the total number of data items in the binary format.
     * @param totalDataItems The total number of data items.
     */
    void setTotalDataItems(int totalDataItems);

    /**
     * @brief Sets the time resolution for the binary format.
     * @param timeResolution The time resolution in seconds.
     */
    void setTimeResolution(double timeResolution);

    /**
     * @brief Adds a data item to the binary format.
     * @param name The name of the data item.
     * @param dataItem The `DataItem` object containing metadata for the data item.
     */
    void addDataItem(const std::string& name, const DataItem& dataItem);


    // Getters
    /**
     * @brief Retrieves the current day for the binary format.
     * @return The current day value.
     */
    int getCurrentDay() const;

    /**
     * @brief Retrieves the total number of data items in the binary format.
     * @return The total number of data items.
     */
    int getTotalDataItems() const;

    /**
     * @brief Retrieves the time resolution for the binary format.
     * @return The time resolution in seconds.
     */
    double getTimeResolution() const;

    /**
     * @brief Retrieves metadata for a specific data item.
     * @param name The name of the data item.
     * @return The `DataItem` object containing metadata for the specified data item.
     * @throws std::runtime_error If the data item is not found.
     */
    DataItem getDataItem(const std::string& name) const;

    // Utility Methods
    /**
     * @brief Checks if a specific data item exists in the binary format.
     * @param name The name of the data item.
     * @return True if the data item exists, false otherwise.
     */
    bool hasDataItem(const std::string& name) const;

    /**
     * @brief Retrieves the names of all data items in the binary format.
     * @return A vector of strings containing the names of all data items.
     */
    std::vector<std::string> getAllDataItemNames() const;

    /**
     * @brief Clears all metadata stored in the binary format.
     */
    void clear();

    /**
     * @brief Prints a summary of the binary format metadata to the console.
     */
    void printSummary() const;

private:
    std::unordered_map<std::string, DataItem> dataItems; ///< A map of data item names to their metadata.
    int currentDay = 0; ///< The current day value for the binary format.
    int totalDataItems = 0; ///< The total number of data items in the binary format.
    double timeResolution = 0.0; ///< The time resolution for the binary format in seconds.
};

#endif // EPITRENDBINARYFORMAT_HPP
