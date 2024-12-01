#ifndef EPITRENDBINARYDATA_HPP
#define EPITRENDBINARYDATA_HPP

#include "Common.hpp"

class EpitrendBinaryData {
public:
    // Constructors
    EpitrendBinaryData() = default;

    // Setters
    void addDataItem(
        std::string name, 
        std::pair<double,double> time_series, 
        bool verbose = false
    );

    // Getters
    std::unordered_map<std::string, std::unordered_map<double,double>> getAllTimeSeriesData();
    int getByteSize();

    // Utility Methods
    void printAllTimeSeriesData();
    void printFileAllTimeSeriesData(const std::string& filename);

    // Clear all contents of time-series data
    void clear();

private:
    std::unordered_map<std::string, std::unordered_map<double,double>> allTimeSeriesData;
    int byteSize = 0;
};

#endif // EPITRENDBINARYDATA_HPP
