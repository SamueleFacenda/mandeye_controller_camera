//
// Created by samu on 06/06/24.
//

#ifndef MANDEYE_MULTISENSOR_SAVECHUNKTODIRCLIENT_H
#define MANDEYE_MULTISENSOR_SAVECHUNKTODIRCLIENT_H

#include <filesystem>
namespace mandeye_utils
{

class SaveChunkToDirClient
{
public:
	virtual void saveChunkToDirectory(const std::filesystem::path& directory, int chunk) = 0;
};

} // namespace mandeye_utils

#endif //MANDEYE_MULTISENSOR_SAVECHUNKTODIRCLIENT_H
