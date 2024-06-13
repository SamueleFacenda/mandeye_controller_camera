#ifndef MANDEYE_MULTISENSOR_SAVECHUNKTODIRCLIENT_H
#define MANDEYE_MULTISENSOR_SAVECHUNKTODIRCLIENT_H

#include <filesystem>
namespace mandeye
{

class SaveChunkToDirClient
{
public:
	virtual void saveChunkToDirectory(const std::filesystem::path& directory, int chunk) = 0;
};

} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_SAVECHUNKTODIRCLIENT_H
