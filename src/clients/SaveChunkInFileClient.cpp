//
// Created by samu on 06/06/24.
//

#include "clients/SaveChunkInFileClient.h"
#include <fstream>
#include <iostream>

namespace mandeye_utils
{

void SaveChunkInFileClient::saveChunkToDirectory(const std::filesystem::path& directory, int chunk) {
	std::ofstream outs;
	bool retFileOpen = getSavingStream(outs, directory, chunk);
	if (!retFileOpen)
		return;
	std::stringstream oss;
	printBufferToFileString(oss);
	outs << oss.rdbuf();
	outs.close();
}


bool SaveChunkInFileClient::getSavingStream(std::ofstream& out, const std::string& directory, int chunkNumber)
{
	char filename[64];
	snprintf(filename, 64, "%s%04d.%s", getFileIdentifier().c_str(), chunkNumber, getFileExtension().c_str());
	using namespace std::filesystem;
	path outFile = path(directory) / path(filename);
	out.open(outFile);
	if(out.fail())
	{
		std::cerr << "Error opening file '" << filename << "' !!" << std::endl;
		out.close();
		return false;
	}
	return true;
}

} // namespace mandeye_utils