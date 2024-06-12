#ifndef MANDEYE_MULTISENSOR_IMUFILESAVER_H
#define MANDEYE_MULTISENSOR_IMUFILESAVER_H

#include "clients/SaveChunkInFileClient.h"
#include "livox_types.h"

namespace mandeye
{

class ImuFileSaver : public SaveChunkInFileClient
{
private:
	LivoxIMUBufferPtr buffer{};

public:
	void setBuffer(const LivoxIMUBufferPtr& inBuf);

protected:
	void printBufferToFileString(std::stringstream& fss) override;
	std::string getFileExtension() override;
	std::string getFileIdentifier() override;
};

} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_IMUFILESAVER_H
