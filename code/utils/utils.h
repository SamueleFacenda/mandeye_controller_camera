#ifndef MANDEYE_MULTISENSOR_UTILS_H
#define MANDEYE_MULTISENSOR_UTILS_H
#include "clients/GpioClient.h"

namespace utils
{
std::string getEnvString(const std::string& env, const std::string& def);
bool getEnvBool(const std::string& env, bool def);
void blinkLed(mandeye::GpioClient::LED led, std::chrono::milliseconds mills);
void syncDisk();
}
#endif //MANDEYE_MULTISENSOR_UTILS_H
