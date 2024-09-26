#pragma once

#include <cstdint>

namespace mandeye
{
//! Interface for a class that provides a timestamp
class TimeStampProvider
{
public:
	virtual uint64_t getTimestamp() = 0;
};
} // namespace mandeye