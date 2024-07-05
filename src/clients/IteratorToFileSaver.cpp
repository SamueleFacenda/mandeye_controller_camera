#include "clients/IteratorToFileSaver.h"

namespace mandeye
{

template <template <typename...> class Container, typename... Args>
IteratorToFileSaver<Container, Args...>::IteratorToFileSaver(std::string fileExtension, std::string fileIdentifier, Formatter func)
	: fileExtension(std::move(fileExtension)), fileIdentifier(std::move(fileIdentifier)), func(func) {}

template <template <typename...> class Container, typename... Args>
void IteratorToFileSaver<Container,Args...>::setBuffer(Iterator newStart, Iterator newEnd)
{
	this->start = newStart;
	this->end = newEnd;
}

template <template <typename...> class Container, typename... Args>
void IteratorToFileSaver<Container, Args...>::printBufferToFileString(std::stringstream& fss)
{
	for (auto it = start; it != end; ++it)
		fss << func(*it);
}

template <template <typename...> class Container, typename... Args>
std::string IteratorToFileSaver<Container, Args...>::getFileExtension()
{
	return fileExtension;
}

template <template <typename...> class Container, typename... Args>
std::string IteratorToFileSaver<Container, Args...>::getFileIdentifier()
{
	return fileIdentifier;
}

} // namespace mandeye