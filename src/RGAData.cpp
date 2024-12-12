#include "RGAData.hpp"

void RGAData::addData(const RGAData::AMUBins& bins, double time, double value) {
    allTimeSeriesData[bins][time] = value;
}

const std::unordered_map<RGAData::AMUBins, std::unordered_map<double,double>,RGAData::AMUBinsHash>& RGAData::getData() {
    return allTimeSeriesData;
}

void RGAData::clearData() {
    allTimeSeriesData.clear();
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
