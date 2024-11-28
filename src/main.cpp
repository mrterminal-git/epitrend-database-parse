#include "Common.hpp"
#include "FileReader.hpp"
#include "Config.hpp"

int main() {

// Testing reading epitrend binary format files
FileReader::parseEpitrendFile(2024,11,1,1,false);

return 0;
}
