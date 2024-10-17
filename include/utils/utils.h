#ifndef MANDEYE_MULTISENSOR_UTILS_H
#define MANDEYE_MULTISENSOR_UTILS_H
#include "clients/concrete/GpioClient.h"

namespace utils
{
std::string getEnvString(const std::string& env, const std::string& def);
bool getEnvBool(const std::string& env, bool def);
void blinkLed(mandeye::LED led, std::chrono::milliseconds mills);
void syncDisk();
std::vector<int> getIntListFromEnvVar(const std::string& env, const std::string& def);
}
#endif //MANDEYE_MULTISENSOR_UTILS_H
