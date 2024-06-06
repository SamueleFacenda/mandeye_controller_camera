#ifndef MANDEYE_MULTISENSOR_STATE_MANAGEMENT_H
#define MANDEYE_MULTISENSOR_STATE_MANAGEMENT_H

#include "clients/CamerasClient.h"
#include "clients/FileSystemClient.h"
#include "clients/GnssClient.h"
#include "clients/GpioClient.h"
#include "clients/LivoxClient.h"
#include "utils/save_laz.h"
#include <iostream>
#include <string>

namespace mandeye
{
enum class States
{
	WAIT_FOR_RESOURCES = -10,
	IDLE = 0,
	STARTING_SCAN = 10,
	SCANNING = 20,
	STOPPING = 30,
	STOPPED = 40,
	STARTING_STOP_SCAN = 100,
	STOP_SCAN_IN_PROGRESS = 150,
	STOP_SCAN_IN_INITIAL_PROGRESS = 160,
	STOPING_STOP_SCAN = 190,
	LIDAR_ERROR = 200,
	USB_IO_ERROR = 210,
};

const std::map<States, std::string> StatesToString{
	{States::WAIT_FOR_RESOURCES, "WAIT_FOR_RESOURCES"},
	{States::IDLE, "IDLE"},
	{States::STARTING_SCAN, "STARTING_SCAN"},
	{States::SCANNING, "SCANNING"},
	{States::STOPPING, "STOPPING"},
	{States::STOPPED, "STOPPED"},
	{States::STARTING_STOP_SCAN, "STARTING_STOP_SCAN"},
	{States::STOP_SCAN_IN_PROGRESS, "STOP_SCAN_IN_PROGRESS"},
	{States::STOP_SCAN_IN_INITIAL_PROGRESS, "STOP_SCAN_IN_INITIAL_PROGRESS"},
	{States::STOPING_STOP_SCAN, "STOPING_STOP_SCAN"},
	{States::LIDAR_ERROR, "LIDAR_ERROR"},
	{States::USB_IO_ERROR, "USB_IO_ERROR"},
};

extern std::atomic<bool> isRunning;
extern std::mutex livoxClientPtrLock;
extern std::shared_ptr<LivoxClient> livoxClientPtr;
extern std::shared_ptr<GNSSClient> gnssClientPtr;
extern std::mutex gpioClientPtrLock;
extern std::shared_ptr<GpioClient> gpioClientPtr;
extern std::shared_ptr<FileSystemClient> fileSystemClientPtr;
extern std::shared_ptr<CamerasClient> camerasClientPtr;

extern States app_state;

std::string produceReport();
bool StartScan();
bool StopScan();

bool TriggerStopScan();
bool TriggerContinousScanning();
void savePointcloudData(const LivoxPointsBufferPtr& buffer, const std::string& directory, int chunk);
void saveLidarList(const std::unordered_map<uint32_t, std::string> &lidars, const std::string& directory, int chunk);
void saveImuData(const LivoxIMUBufferPtr& buffer, const std::string& directory, int chunk);
void saveGnssData(std::deque<std::string>& buffer, const std::string& directory, int chunk);
bool saveChunkToDisk(const std::string& outDirectory, int chunk);
void getSavingStream(std::ofstream& out, const std::string& directory, const std::string& fileExtension, int chunkNumber);

void stateWatcher();
} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_STATE_MANAGEMENT_H
