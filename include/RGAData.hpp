#ifndef RGADATA_HPP
#define RGADATA_HPP

#include "Common.hpp"
#include "Config.hpp"

/**
 * @class RGAData
 * @brief A class for managing and processing time-series data from RGA (Residual Gas Analyzer) systems.
 *
 * The `RGAData` class provides functionality to store, manipulate, and retrieve
 * time-series data organized into AMU (Atomic Mass Unit) bins. It supports operations
 * such as adding data, clearing data, calculating differences, and printing data.
 */
class RGAData {
public:
    /**
     * @brief Tolerance for floating-point comparisons.
     */
    constexpr static double TOLERANCE = 1e-9;

    /**
     * @struct FloatCompare
     * @brief Custom comparison function for floating-point numbers.
     *
     * Compares two floating-point numbers with a defined tolerance to account for precision errors.
     */
    struct FloatCompare {
        bool operator()(double lhs, double rhs) const {
            return std::fabs(lhs - rhs) < TOLERANCE;
        }
    };

    /**
     * @struct AMUBins
     * @brief Represents a set of AMU bins and their associated Growth Module.
     *
     * The `AMUBins` struct stores a set of AMU bins in ascending order and provides
     * utility functions for comparison, hashing, and string representation.
     */
    struct AMUBins {
        /**
         * @brief A set of AMU bins in ascending order.
         */
        std::set<double, std::less<>> bins;
        
        /**
         * @brief Default GM (Growth Module) associated with the bins.
         */
        std::string GM = "Cluster";
        
        /**
         * @brief Constructs an AMUBins object with the given bin values.
         * @param bin_values A vector of bin values to initialize the bins.
         */
        AMUBins(const std::vector<double>& bin_values) {
            bins.insert(bin_values.begin(), bin_values.end());
        }

        /**
         * @brief Equality operator for comparing two AMUBins objects.
         * @param other The other AMUBins object to compare.
         * @return True if the bins and GM are equal, false otherwise.
         */
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

        /**
         * @brief Generates a string representation of the bins.
         * @return A string containing the GM and bin values.
         */
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

        /**
         * @brief Prints the bins to the console.
         */
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

    /**
     * @struct AMUBinsHash
     * @brief Custom hash function for AMUBins objects.
     *
     * Generates a hash value for an AMUBins object based on its bin values.
     */
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
    /**
     * @brief Default constructor for the RGAData class.
     */
    RGAData() = default;

    /**
     * @brief Constructs an RGAData object with a specified number of bins per unit.
     * @param bins_per_unit The number of bins per unit.
     * @throws std::runtime_error If bins_per_unit exceeds 9.
     */
    RGAData(const int& bins_per_unit);


    // Getters and Setters
    /**
     * @brief Adds a time-series data point to the specified AMUBins.
     * @param bins The AMUBins object representing the bins.
     * @param time The timestamp of the data point.
     * @param value The value of the data point.
     */
    void addData(const AMUBins& bins, double time, double value);

    /**
     * @brief Retrieves the total byte size of the RGAData object.
     * @return The byte size of the object, including all stored data.
     */
    int getByteSize() const;

    /**
     * @brief Retrieves all time-series data stored in the object.
     * @return A map containing all time-series data, where the key is the AMUBins and the value is a map of timestamps and values.
     */
    const std::unordered_map<AMUBins, std::unordered_map<double, double, std::hash<double>, std::equal_to<double>, std::allocator<std::pair<const double, double>>>, AMUBinsHash, std::equal_to<AMUBins>, std::allocator<std::pair<const AMUBins, std::unordered_map<double, double, std::hash<double>, std::equal_to<double>, std::allocator<std::pair<const double, double>>>>>>& getAllTimeSeriesData() const;
    
    /**
     * @brief Retrieves all AMUBins stored in the object.
     * @return A vector of AMUBins objects.
     */
    const std::vector<AMUBins> getBins();


    // Utility
    /**
     * @brief Prints all time-series data to the console.
     */
    void printAllTimeSeriesData();

    /**
     * @brief Writes all time-series data to a file.
     * @param config The configuration object containing the output directory.
     * @param filename The name of the file to write the data to.
     */
    void printFileAllTimeSeriesData(const Config& config, const std::string& filename);
    
     /**
     * @brief Clears all time-series data from the object.
     */
    void clearData();

    /**
     * @brief Calculates the difference between two RGAData objects.
     * @param other The other RGAData object to compare against.
     * @return A new RGAData object containing the difference.
     */
    RGAData difference(const RGAData& other) const;

    /**
     * @brief Checks if the object contains any time-series data.
     * @return True if the object is empty, false otherwise.
     */
    bool is_empty() const;

private:
    /**
     * @brief A map to store all time-series data.
     * The key is the AMUBins object, and the value is a map of timestamps and values.
     */
    std::unordered_map<AMUBins, std::unordered_map<double, double>, AMUBinsHash> allTimeSeriesData;

    /**
     * @brief The total byte size of the object, including all stored data.
     */
    int byteSize = 0;
};

#endif // RGADATA_HPP