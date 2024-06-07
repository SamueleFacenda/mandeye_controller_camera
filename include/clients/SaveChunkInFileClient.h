//
// Created by samu on 06/06/24.
//

#ifndef MANDEYE_MULTISENSOR_SAVECHUNKINFILECLIENT_H
#define MANDEYE_MULTISENSOR_SAVECHUNKINFILECLIENT_H

#include "clients/SaveChunkToDirClient.h"

namespace mandeye_utils
{

class SaveChunkInFileClient: public SaveChunkToDirClient
{
public:
	void saveChunkToDirectory(const std::filesystem::path& directory, int chunk) override;

protected:
	virtual void printBufferToFileString(std::stringstream& fss) = 0;
	virtual std::string getFileExtension() = 0;
	virtual std::string getFileIdentifier() = 0;

private:
	bool getSavingStream(std::ofstream& out, const std::string& directory, int chunkNumber);

};

}

#endif //MANDEYE_MULTISENSOR_SAVECHUNKINFILECLIENT_H
