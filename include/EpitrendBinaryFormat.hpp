#ifndef EPITRENDBINARYFORMAT_HPP
#define EPITRENDBINARYFORMAT_HPP

#include "Common.hpp"

class EpitrendBinaryFormat {
public:
    struct dataItem {
        std::string Name;
        std::string Type;
        std::string Range;
        int TotalValues;
        int ValueOffset;
    };

    EpitrendBinaryFormat() = default;

    void setCurrentDay(int current_day) {currentDay = current_day;}
    void setTotalDataItems(int total_data_items) {totalDataItems = total_data_items;}
    void setTimeResoltuion(int time_resolution) {timeResolution = time_resolution;}
    void setDataItems(std::unordered_map<std::string, EpitrendBinaryFormat::dataItem> data_items) {dataItems = data_items;}
    void addDataItem(std::string name, dataItem data_item) {dataItems[name] = data_item;}


private:
    std::unordered_map<std::string, dataItem> dataItems;
    int currentDay;
    int totalDataItems;
    double timeResolution;

};

#endif // EPITRENDBINARYFORMAT_HPP
