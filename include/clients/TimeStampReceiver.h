#pragma once
#include <memory>
namespace mandeye
{
class TimeStampProvider;
class TimeStampReceiver
{
public:
	//! Set timestamp provider
	void SetTimeStampProvider(std::shared_ptr<TimeStampProvider> timeStampProvider);
	//! Returns the current timestamp in nanoseconds
	uint64_t GetTimeStamp();
protected:
	//! The timestamp provider
	std::shared_ptr<TimeStampProvider> m_timeStampProvider;
};
} // namespace mandeye
