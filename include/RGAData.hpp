#ifndef RGADATA_HPP
#define RGADATA_HPP

#include "Common.hpp"

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
        std::set<double, std::less<>> bins;

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
            return true;
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

    // Getters and Setters
    void addData(const AMUBins& bins, double time, double value);

    const std::unordered_map<AMUBins, std::unordered_map<double, double>, AMUBinsHash>& getData();

    // Utility
    void printAllTimeSeriesData();
    void clearData();

private:
    std::unordered_map<AMUBins, std::unordered_map<double, double>, AMUBinsHash> allTimeSeriesData;
    int byteSize = 0;
};

#endif // RGADATA_HPP