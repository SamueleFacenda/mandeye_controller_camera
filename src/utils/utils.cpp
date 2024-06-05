#include "utils/utils.h"
#include "state_management.h"

namespace utils
{
std::string getEnvString(const std::string& env, const std::string& def)
{
	const char* env_p = std::getenv(env.c_str());
	if(env_p == nullptr)
	{
		return def;
	}
	return std::string{env_p};
}

bool getEnvBool(const std::string& env, bool def)
{
	const char* env_p = std::getenv(env.c_str());
	if(env_p == nullptr)
	{
		return def;
	}
	if(strcmp("1", env_p) == 0 || strcmp("true", env_p) == 0)
	{
		return true;
	}
	return false;
}

void blinkLed(mandeye::GpioClient::LED led, std::chrono::milliseconds mills) {
	mandeye::gpioClientPtr->setLed(led, true);
	std::this_thread::sleep_for(mills);
	mandeye::gpioClientPtr->setLed(led, false);
	std::this_thread::sleep_for(mills);
}

void syncDisk() {
	if (system("sync")) {
		std::cerr << "Error syncing disk\n";
	}
}
}
