#ifndef RGADATA_HPP
#define RGADATA_HPP

#include "Common.hpp"
#include "Config.hpp"

class RGAData {
public:
    // Custom struct for hash function
    // Define the tolerance for floating-point comparison
    constexpr static double TOLERANCE = 1e-9;

    // Custom comparison function for floating-point numbers
    struct FloatCompare {
        bool operator()(double lhs, double rhs) const {
            return std::fabs(lhs - rhs) < TOLERANCE;
        }
    };

    // Define the AMUBins struct
    struct AMUBins {
        // AMU bins in acending order
        std::set<double, std::less<>> bins;
        
        // AMUBins default GM
        std::string GM = "Cluster";
        
        // Constructor to initialize the bins
        AMUBins(const std::vector<double>& bin_values) {
            bins.insert(bin_values.begin(), bin_values.end());
        }

        // Equality operator
        bool operator==(const AMUBins& other) const {
            if (bins.size() != other.bins.size()) {
                return false;
            }
            auto it1 = bins.begin();
            auto it2 = other.bins.begin();
            while (it1 != bins.end() && it2 != other.bins.end()) {
                if (!FloatCompare()(*it1, *it2)) {
                    return false;
                }
                ++it1;
                ++it2;
            }
            // Last check to see if GM is the same
            return GM == other.GM;
            return true;
        }

        // Bins string
        std::string binsString() const {
            std::string bin_string = GM + ".";
            for (const auto& bin : bins) {
                std::ostringstream bin_stream;
                bin_stream.precision(2);
                bin_stream << std::fixed << bin;
                std::string bin_precise = std::move(bin_stream).str();
                bin_string += "" + bin_precise + "_";
            }
            if (!bin_string.empty()) {
                bin_string.pop_back();
            }
            return bin_string;
        }

        // Print function
        void print() const {
            std::string bin_string = "";
            for (const auto& bin : bins) {
                bin_string += "" + std::to_string(bin) + ",";
            }
            if (!bin_string.empty()) {
                bin_string.pop_back();
            }
            std::cout << bin_string << "\n";
        }
    };

    // Custom hash function for AMUBins
    struct AMUBinsHash {
        std::size_t operator()(const AMUBins& amubins) const {
            std::size_t seed = 0;
            for (const auto& bin : amubins.bins) {
                seed ^= std::hash<double>()(bin) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };

public:
    // Constructors
    RGAData() = default;
    RGAData(const int& bins_per_unit);

    // Getters and Setters
    void addData(const AMUBins& bins, double time, double value);
    int getByteSize() const;
    const std::unordered_map<AMUBins, std::unordered_map<double, double, std::hash<double>, std::equal_to<double>, std::allocator<std::pair<const double, double>>>, AMUBinsHash, std::equal_to<AMUBins>, std::allocator<std::pair<const AMUBins, std::unordered_map<double, double, std::hash<double>, std::equal_to<double>, std::allocator<std::pair<const double, double>>>>>>& getAllTimeSeriesData() const;
    const std::vector<AMUBins> getBins();

    // Utility
    void printAllTimeSeriesData();
    void printFileAllTimeSeriesData(const Config& config, const std::string& filename);
    void clearData();
    RGAData difference(const RGAData& other) const;
    bool is_empty() const;

private:
    std::unordered_map<AMUBins, std::unordered_map<double, double>, AMUBinsHash> allTimeSeriesData;
    int byteSize = 0;
};

#endif // RGADATA_HPP