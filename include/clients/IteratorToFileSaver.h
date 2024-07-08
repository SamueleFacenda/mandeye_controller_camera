#ifndef MANDEYE_MULTISENSOR_ITERATORTOFILESAVER_H
#define MANDEYE_MULTISENSOR_ITERATORTOFILESAVER_H

#include <utility>
#include <functional>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace mandeye
{

template <template <typename...> class Container, typename... Args>
class IteratorToFileSaver
{
public:
	using Iterator = typename Container<Args...>::iterator;
	using Formatter = std::function<std::string (typename Container<Args...>::value_type)>;

	IteratorToFileSaver(std::string fileExtension, std::string fileIdentifier, Formatter formatter)
		: fileExtension(std::move(fileExtension)), fileIdentifier(std::move(fileIdentifier)), formatter(formatter) {};
	void setBuffer(Iterator newStart, Iterator newEnd) {
		this->start = newStart;
		this->end = newEnd;
	};

	void saveDumpedChunkToDirectory(const std::filesystem::path& directory, int chunk) {
		std::ofstream outs;
		bool retFileOpen = getSavingStream(outs, directory, chunk);
		if (!retFileOpen)
			return;
		for(auto it = start; it != end; ++it)
			outs << formatter(*it) << std::endl;
		outs.close();
	}

private:
	std::string fileExtension;
	std::string fileIdentifier;
	Iterator start, end;
	Formatter formatter;

	bool getSavingStream(std::ofstream& out, const std::string& directory, int chunkNumber) {
		// auto filename = std::format("{}{:04d}.{}", getFileIdentifier(), chunkNumber, getFileExtension());
		// not supported yet (livoxsdk uses old c++)
		char filename[64];
		snprintf(filename, 64, "%s%04d.%s", fileIdentifier.c_str(), chunkNumber, fileExtension.c_str());
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
};


} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_ITERATORTOFILESAVER_H
