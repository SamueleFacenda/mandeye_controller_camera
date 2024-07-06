#ifndef MANDEYE_MULTISENSOR_ITERATORTOFILESAVER_H
#define MANDEYE_MULTISENSOR_ITERATORTOFILESAVER_H

#include <utility>
#include <functional>

#include "clients/SaveChunkInFileClient.h"

namespace mandeye
{

template <template <typename...> class Container, typename... Args>
class IteratorToFileSaver : public SaveChunkInFileClient
{
public:
	using Iterator = typename Container<Args...>::iterator;
	using Formatter = std::function<std::string (typename Container<Args...>::value_type)>;

	IteratorToFileSaver(std::string fileExtension, std::string fileIdentifier, Formatter formatter)
		: fileExtension(std::move(fileExtension)), fileIdentifier(std::move(fileIdentifier)), formatter(formatter) {};
	void dumpChunkInternally() override {};
	void setBuffer(Iterator newStart, Iterator newEnd) {
		this->start = newStart;
		this->end = newEnd;
	};

protected:
	void printBufferToFileString(std::stringstream& fss) override {
		for (auto it = start; it != end; ++it)
			fss << formatter(*it) << std::endl;
	};
	std::string getFileExtension() override { return fileExtension; };
	std::string getFileIdentifier() override { return fileIdentifier; };

private:
	std::string fileExtension;
	std::string fileIdentifier;
	Iterator start, end;
	Formatter formatter;
};


} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_ITERATORTOFILESAVER_H
