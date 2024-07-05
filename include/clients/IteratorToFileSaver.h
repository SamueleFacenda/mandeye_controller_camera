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
	using Formatter = std::function<std::string (Args...)>;

	IteratorToFileSaver(std::string fileExtension, std::string fileIdentifier, Formatter func);
	void dumpChunkInternally() override {};
	void setBuffer(Iterator newStart, Iterator newEnd);

protected:
	void printBufferToFileString(std::stringstream& fss) override;
	std::string getFileExtension() override;
	std::string getFileIdentifier() override;

private:
	std::string fileExtension;
	std::string fileIdentifier;
	Iterator start, end;
	Formatter func;
};

} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_ITERATORTOFILESAVER_H
