#ifndef MANDEYE_MULTISENSOR_LIDARFILESAVER_H
#define MANDEYE_MULTISENSOR_LIDARFILESAVER_H

#include "clients/SaveChunkInFileClient.h"
#include <unordered_map>

namespace mandeye
{

class LidarFileSaver : public SaveChunkInFileClient
{
private:
	std::unordered_map<uint32_t, std::string> buffer;

public:
	void setBuffer(const std::unordered_map<uint32_t, std::string>& inBuf);
	void dumpChunkInternally() override {};

protected:
	void printBufferToFileString(std::stringstream& fss) override;
	std::string getFileExtension() override;
	std::string getFileIdentifier() override;
};

}

#endif //MANDEYE_MULTISENSOR_LIDARFILESAVER_H
