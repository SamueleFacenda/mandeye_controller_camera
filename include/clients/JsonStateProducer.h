#ifndef MANDEYE_MULTISENSOR_JSONSTATEPRODUCER_H
#define MANDEYE_MULTISENSOR_JSONSTATEPRODUCER_H

#include <json.hpp>

namespace mandeye_utils
{

class JsonStateProducer
{
public:
	virtual nlohmann::json produceStatus() = 0;
	virtual std::string getJsonName() = 0;
};

} // namespace mandeye_utils

#endif //MANDEYE_MULTISENSOR_JSONSTATEPRODUCER_H
