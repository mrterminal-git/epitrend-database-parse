#include "EpitrendBinaryData.hpp"

// Setters
void EpitrendBinaryData::addDataItem( std::string name, 
    std::pair<double,double> time_series, 
    bool verbose
) { 
    // Check if name already exists in map
    if (allTimeSeriesData.find(name) == allTimeSeriesData.end()) {
        // Key does not exist so add name and time-series data
        allTimeSeriesData[name][time_series.first]= time_series.second;

    } else {
        // Key exists so append to the time-series data
        // Check if any time-data already exists for the name
        if (allTimeSeriesData[name].find(time_series.first) == allTimeSeriesData[name].end()) {
            // Time-data does not exist for the name so add to the map
            allTimeSeriesData[name][time_series.first] = time_series.second;

        } else {
            // Time-data does exist so replace the data but also give out warning (if verbose was given)
            allTimeSeriesData[name][time_series.first] = time_series.second;

            // Give user a warning
            if (verbose) {
                std::cerr << std::setprecision(15) << "Warning in EpitrendBinaryData::addDataItem call: time-series data " << time_series.first << " already exists for " << name << ".\n";
                
            }
        
        }
    
    }

    // Increment byteSize of object
    byteSize = byteSize + 16 + name.length(); // 8 bytes * 2 + 1 byte * number of chars
}

// Getters
std::unordered_map<std::string, std::unordered_map<double,double>> EpitrendBinaryData::getAllTimeSeriesData() {
    return allTimeSeriesData; 
}
int EpitrendBinaryData::getByteSize() { return byteSize;}

// Utility
void EpitrendBinaryData::printAllTimeSeriesData(){
        for(auto element : allTimeSeriesData){
            std::cout << element.first << "\n";
            
            for(auto inner_element : element.second){
                std::cout << "  " << inner_element.first << "," << inner_element.second << "\n";
            }
        }
}

void EpitrendBinaryData::printFileAllTimeSeriesData(const std::string& filename){
    std::string fullpath = Config::getOutputDir() + filename;
    std::ofstream outFile(fullpath);
    for(auto element : allTimeSeriesData) {
        outFile << element.first << "\n";
        
        for(auto inner_element : element.second) {
            outFile << std::setprecision(15) << inner_element.first << "," << inner_element.second << "\n";
        
        }

    }
}

bool EpitrendBinaryData::is_empty(){
    return allTimeSeriesData.empty();
}

//
void EpitrendBinaryData::clear(){
    allTimeSeriesData.clear();
    byteSize = 0;
}

// Return the difference between two EpitrendBinaryData objects
EpitrendBinaryData EpitrendBinaryData::difference(EpitrendBinaryData& other) const{
    std::unordered_map<std::string, std::unordered_map<double,double>> other_data = other.getAllTimeSeriesData();
    EpitrendBinaryData diff_data;
    for(auto element : allTimeSeriesData){
        std::string name = element.first;
        std::unordered_map<double,double> time_series = element.second;
        for(auto inner_element : time_series){
            double time = inner_element.first;
            double value = inner_element.second;
            if(other_data.find(name) == other_data.end() || other_data[name].find(time) == other_data[name].end()){
                diff_data.addDataItem(name, std::make_pair(time, value));
            }
        }
    }

    return diff_data;
    
}
