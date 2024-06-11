#ifndef MANDEYE_MULTISENSOR_IMUFILESAVER_H
#define MANDEYE_MULTISENSOR_IMUFILESAVER_H

#include "clients/SaveChunkInFileClient.h"
#include "livox_types.h"

namespace mandeye_utils
{

class ImuFileSaver : public SaveChunkInFileClient
{
private:
	mandeye::LivoxIMUBufferPtr buffer{};

public:
	void setBuffer(const mandeye::LivoxIMUBufferPtr& inBuf);

protected:
	void printBufferToFileString(std::stringstream& fss) override;
	std::string getFileExtension() override;
	std::string getFileIdentifier() override;
};

} // namespace mandeye_utils

#endif //MANDEYE_MULTISENSOR_IMUFILESAVER_H
