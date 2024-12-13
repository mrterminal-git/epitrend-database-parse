#include "RGAData.hpp"

// Default constructor
RGAData::RGAData() {
    // Initialize bins around integers 1 to 99
    for (int i = 1; i <= 99; ++i) {
        std::vector<double> bin_values;
        for (double j = -0.4; j <= 0.4; j += 0.1) {
            bin_values.push_back(i + j);
        }
        AMUBins amubins(bin_values);
        allTimeSeriesData[amubins] = std::unordered_map<double, double>();
    }
}
RGAData::RGAData(const int& bins_per_unit) {
    // Force that bins_per_unit must be less than 5
    if (bins_per_unit > 4) {
        throw std::runtime_error("Error in RGAData constructor: bins per unit must be less than 5");
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

void RGAData::printFileAllTimeSeriesData(const std::string& filename) {
    std::ofstream outFile(Config::getOutputDir() + filename);
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
