#include "Common.hpp"
#include "FileReader.hpp"
#include "Config.hpp"
#include "EpitrendBinaryFormat.hpp"
#include "EpitrendBinaryData.hpp"

int main() {

// Testing parsing the binary file using the binary format file
// EpitrendBinaryData object
// using the EpitrendBinaryFormat object, extract the EpitrendData object

// EpitrendBinaryData binaryData;
// binaryData.addDataItem("Parameter1", {0.1, 2});
// binaryData.addDataItem("Parameter1", {0.2, 3});
// binaryData.addDataItem("Parameter1", {0.1, 1});
// binaryData.addDataItem("Parameter2", {0.1, 2});

// binaryData.printAllTimeSeriesData();

EpitrendBinaryData binary_data;
for (int i = 0; i < 24; i++) {
    FileReader::parseEpitrendBinaryDataFile(binary_data,2024,11,1,i,false);
    std::cout << "Current size of EpitrendBinaryData object: " << binary_data.getByteSize() << "\n";
}
binary_data.printFileAllTimeSeriesData("temp.txt");


return 0;
}
