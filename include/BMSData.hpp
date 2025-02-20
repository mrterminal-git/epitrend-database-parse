#ifndef BMSDATA_CPP
#define BMSDATA_CPP

#include "Common.hpp"

class BMSData {
public:
    // Constructors
    BMSData() = default;

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
    bool is_empty();

    // Clear all contents of time-series data
    void clear();

    // Difference between two EpitrendBinaryData objects
    BMSData difference(BMSData& other) const;


private:
    std::unordered_map<std::string, std::unordered_map<double,double>> allTimeSeriesData;
    int byteSize = 0;
};

#endif // EPITRENDBINARYDATA_HPP
