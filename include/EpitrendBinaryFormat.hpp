#ifndef EPITRENDBINARYFORMAT_HPP
#define EPITRENDBINARYFORMAT_HPP

#include "Common.hpp"

class EpitrendBinaryFormat {
public:
    struct DataItem {
        std::string Name;
        std::string Type;
        std::string Range;
        int TotalValues = 0;
        int ValueOffset = 0;

        friend std::ostream& operator<<(std::ostream& os, const DataItem& item) {
            os << "Name: " << item.Name << ", Type: " << item.Type 
               << ", Range: " << item.Range << ", TotalValues: " 
               << item.TotalValues << ", ValueOffset: " << item.ValueOffset;
            return os;
        }
    };

    // Constructors
    EpitrendBinaryFormat() = default;

    // Setters
    void setCurrentDay(int currentDay);
    void setTotalDataItems(int totalDataItems);
    void setTimeResolution(double timeResolution);
    void addDataItem(const std::string& name, const DataItem& dataItem);

    // Getters
    int getCurrentDay() const;
    int getTotalDataItems() const;
    double getTimeResolution() const;
    DataItem getDataItem(const std::string& name) const;

    // Utility Methods
    bool hasDataItem(const std::string& name) const;
    std::vector<std::string> getAllDataItemNames() const;
    void clear();
    void printSummary() const;

private:
    std::unordered_map<std::string, DataItem> dataItems;
    int currentDay = 0;
    int totalDataItems = 0;
    double timeResolution = 0.0;
};

#endif // EPITRENDBINARYFORMAT_HPP
