#pragma once

namespace mandeye
{
//! Interface for a class that provides a timestamp
class TimeStampProvider
{
public:
	virtual double getTimestamp() = 0;
};
} // namespace mandeye