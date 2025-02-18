#include "RGAData.hpp"

// Constructor with bins per unit
RGAData::RGAData(const int& bins_per_unit) {
    // Force that bins_per_unit must be less than 5
    if (bins_per_unit > 9) {
        throw std::runtime_error("Error in RGAData constructor: bins per unit must be less than 10");
    }

    // Initialize bins around integers 1 to 99
    for (int i = 1; i <= 99; ++i) {
        std::vector<double> bin_values;
        for (int j = 0; j < 2 * bins_per_unit + 1; ++j) {
            bin_values.push_back(i - (double) bins_per_unit * 0.1 + (double) j * 0.1);
        }
        AMUBins amubins(bin_values);
        allTimeSeriesData[amubins] = std::unordered_map<double, double>();
    }
}

void RGAData::addData(const RGAData::AMUBins& bins, double time, double value) {
    allTimeSeriesData[bins][time] = value;

    // Increment byteSize of object
    byteSize = byteSize + 16 + bins.bins.size() * 8; // 8 bytes * 2 + 8 bytes * number of bins
}

int RGAData::getByteSize() const {
    return byteSize;
}

const std::unordered_map<RGAData::AMUBins, std::unordered_map<double, double, std::hash<double>, std::equal_to<double>, std::allocator<std::pair<const double, double>>>, RGAData::AMUBinsHash, std::equal_to<RGAData::AMUBins>, std::allocator<std::pair<const RGAData::AMUBins, std::unordered_map<double, double, std::hash<double>, std::equal_to<double>, std::allocator<std::pair<const double, double>>>>>>& RGAData::getAllTimeSeriesData() const {
    return allTimeSeriesData;
}

const std::vector<RGAData::AMUBins> RGAData::getBins() {
    std::vector<RGAData::AMUBins> bins;
    for(auto element : allTimeSeriesData){
        bins.push_back(element.first);
    }
    return bins;
}

void RGAData::clearData() {
    // Clear all the map within each AMUBins
    for(auto element : allTimeSeriesData){
        element.second.clear();
    }
    
    // Reset byteSize
    byteSize = 0;
}

void RGAData::printAllTimeSeriesData(){
    for(auto element : allTimeSeriesData){
        std::string bin_str = "";
        for(auto bin : element.first.bins){
            bin_str += std::to_string(bin) + ",";
        }
        // Remove the last comma
        if(!bin_str.empty()) bin_str.pop_back();
        std::cout << bin_str << "\n";
        
        for(auto inner_element : element.second){
            std::cout << "  " << inner_element.first << "," << inner_element.second << "\n";
        }
    }
}

void RGAData::printFileAllTimeSeriesData(const Config& config, const std::string& filename) {
    std::ofstream outFile(config.getOutputDir() + filename);
    for(auto element : allTimeSeriesData) {
        std::string bin_str = "";
        for(auto bin : element.first.bins){
            bin_str += std::to_string(bin) + ",";
        }
        // Remove the last comma
        if(!bin_str.empty()) bin_str.pop_back();
        outFile << bin_str << "\n";
        
        for(auto inner_element : element.second) {
            outFile << inner_element.first << "," << inner_element.second << "\n";
        }
    }
}

RGAData RGAData::difference(const RGAData& other) const {
    const auto& other_data = other.getAllTimeSeriesData();
    RGAData diff_data;
    for(const auto& element : allTimeSeriesData){
        AMUBins amubin = element.first;
        std::unordered_map<double,double> time_series = element.second;
        for(auto inner_element : time_series){
            double time = inner_element.first;
            double value = inner_element.second;
            if(other_data.find(amubin) == other_data.end() || other_data.at(amubin).find(time) == other_data.at(amubin).end()){
                diff_data.addData(amubin, time, value); // Pass AMUBins type directly
            }
        }
    }

    return diff_data;
}

bool RGAData::is_empty() const {
    return allTimeSeriesData.empty();
}
