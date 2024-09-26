#ifndef MANDEYE_MULTISENSOR_SYSTEMTIMESTAMPPROVIDER_H
#define MANDEYE_MULTISENSOR_SYSTEMTIMESTAMPPROVIDER_H

#include "clients/TimeStampProvider.h"
#include <cstdint>

namespace mandeye
{
//! used only for testing purposes
class SystemTimeStampProvider: public TimeStampProvider
{

public:
	uint64_t getTimestamp() override;
};

} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_SYSTEMTIMESTAMPPROVIDER_H
