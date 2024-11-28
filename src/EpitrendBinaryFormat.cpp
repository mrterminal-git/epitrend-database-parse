#include "EpitrendBinaryFormat.hpp"

// Setters
void EpitrendBinaryFormat::setCurrentDay(int currentDay) {
    this->currentDay = currentDay;
}

void EpitrendBinaryFormat::setTotalDataItems(int totalDataItems) {
    this->totalDataItems = totalDataItems;
}

void EpitrendBinaryFormat::setTimeResolution(double timeResolution) {
    this->timeResolution = timeResolution;
}

void EpitrendBinaryFormat::addDataItem(const std::string& name, const DataItem& dataItem) {
    dataItems[name] = dataItem;
}

// Getters
int EpitrendBinaryFormat::getCurrentDay() const {
    return currentDay;
}

int EpitrendBinaryFormat::getTotalDataItems() const {
    return totalDataItems;
}

double EpitrendBinaryFormat::getTimeResolution() const {
    return timeResolution;
}

EpitrendBinaryFormat::DataItem EpitrendBinaryFormat::getDataItem(const std::string& name) const {
    if (dataItems.find(name) != dataItems.end()) {
        return dataItems.at(name);
    } else {
        throw std::runtime_error("DataItem not found: " + name);
    }
}

// Utility Methods
bool EpitrendBinaryFormat::hasDataItem(const std::string& name) const {
    return dataItems.find(name) != dataItems.end();
}

std::vector<std::string> EpitrendBinaryFormat::getAllDataItemNames() const {
    std::vector<std::string> names;
    for (const auto& pair : dataItems) {
        names.push_back(pair.first);
    }
    return names;
}

void EpitrendBinaryFormat::clear() {
    dataItems.clear();
    currentDay = 0;
    totalDataItems = 0;
    timeResolution = 0.0;
}

void EpitrendBinaryFormat::printSummary() const {
    std::cout << "Epitrend Binary Format Summary:" << std::endl;
    std::cout << "Current Day: " << currentDay << std::endl;
    std::cout << "Total Data Items: " << totalDataItems << std::endl;
    std::cout << "Time Resolution: " << timeResolution << " seconds" << std::endl;
    std::cout << "Data Items:" << std::endl;
    for (const auto& [name, dataItem] : dataItems) {
        std::cout << "  - " << dataItem << std::endl;
    }
}
