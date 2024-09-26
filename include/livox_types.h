#ifndef MANDEYE_MULTISENSOR_LIVOX_TYPES_H
#define MANDEYE_MULTISENSOR_LIVOX_TYPES_H

#include <livox_lidar_def.h>
#include <unordered_map>
#include <memory>
#include <deque>
#include <string>
#include <set>
#include <thread>

namespace mandeye
{

struct LivoxPoint
{
	LivoxLidarCartesianHighRawPoint point;
	uint64_t timestamp;
	uint8_t line_id;
	uint16_t laser_id;
};

struct LivoxIMU
{
	LivoxLidarImuRawPoint point;
	uint64_t timestamp;
	uint16_t laser_id;
};

const std::unordered_map<int32_t, std::string> WorkModeToStr{
	{LivoxLidarWorkMode::kLivoxLidarNormal, "kLivoxLidarNormal"},
	{LivoxLidarWorkMode::kLivoxLidarWakeUp, "kLivoxLidarWakeUp"},
	{LivoxLidarWorkMode::kLivoxLidarSleep, "kLivoxLidarSleep"},
	{LivoxLidarWorkMode::kLivoxLidarError, "kLivoxLidarError"},
	{LivoxLidarWorkMode::kLivoxLidarPowerOnSelfTest, "kLivoxLidarPowerOnSelfTest"},
	{LivoxLidarWorkMode::kLivoxLidarMotorStarting, "kLivoxLidarMotorStarting"},
	{LivoxLidarWorkMode::kLivoxLidarMotorStoping, "kLivoxLidarMotorStoping"},
	{LivoxLidarWorkMode::kLivoxLidarUpgrade, "kLivoxLidarUpgrade"},
	{-1, "FailedToGetWorkMode"},
};

using LivoxPointsBuffer = std::deque<LivoxPoint>;
using LivoxPointsBufferPtr = std::shared_ptr<std::deque<LivoxPoint>>;
using LivoxPointsBufferConstPtr = std::shared_ptr<const std::deque<LivoxPoint>>;
using LivoxIMUBuffer = std::deque<LivoxIMU>;
using LivoxIMUBufferPtr = std::shared_ptr<std::deque<LivoxIMU>>;
using LivoxIMUBufferConstPtr = std::shared_ptr<const std::deque<LivoxIMU>>;
using ThreadMap = std::unordered_map<std::string,std::shared_ptr<std::thread>>;

} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_LIVOX_TYPES_H
