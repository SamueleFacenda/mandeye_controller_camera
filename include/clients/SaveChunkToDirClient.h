#ifndef MANDEYE_MULTISENSOR_SAVECHUNKTODIRCLIENT_H
#define MANDEYE_MULTISENSOR_SAVECHUNKTODIRCLIENT_H

#include <filesystem>

namespace mandeye
{

class SaveChunkToDirClient
{
public:
	//! Used to sync more clients, you dump the chunk for each client and then save it to the directory
	//!  Because dumping to directory is slower and there is no sync between clients
	virtual void dumpChunkInternally() = 0;
	virtual void saveDumpedChunkToDirectory(const std::filesystem::path& directory, int chunk) = 0;
};

} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_SAVECHUNKTODIRCLIENT_H
