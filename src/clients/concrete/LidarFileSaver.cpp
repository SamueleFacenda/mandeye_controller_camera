#include "clients/concrete/LidarFileSaver.h"

namespace mandeye
{

void LidarFileSaver::printBufferToFileString(std::stringstream& fss) {
	auto it = buffer.begin();
	auto itt = *it;
	for(const auto& [id, sn] : buffer)
	{
		fss << id << " " << sn << "\n";
	}
}

std::string LidarFileSaver::getFileExtension()
{
	return "lidar";
}

std::string LidarFileSaver::getFileIdentifier()
{
	return "ls";
}
void LidarFileSaver::setBuffer(const std::unordered_map<uint32_t, std::string>& inBuf) {
	buffer = inBuf;
}

}