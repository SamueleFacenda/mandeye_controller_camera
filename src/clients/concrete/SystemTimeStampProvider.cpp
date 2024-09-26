#include "clients/concrete/SystemTimeStampProvider.h"
#include <chrono>

uint64_t mandeye::SystemTimeStampProvider::getTimestamp()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();\
}