#include "clients/TimeStampReceiver.h"

#include <utility>
#include "clients/TimeStampProvider.h"
namespace mandeye
{

void TimeStampReceiver::SetTimeStampProvider(std::shared_ptr<TimeStampProvider> timeStampProvider)
{
	m_timeStampProvider = std::move(timeStampProvider);
}

double TimeStampReceiver::GetTimeStamp()
{
	if (m_timeStampProvider)
	{
		return m_timeStampProvider->getTimestamp();
	}
	return 0.0;
}


}

