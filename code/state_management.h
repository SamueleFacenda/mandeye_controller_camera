#ifndef MANDEYE_MULTISENSOR_STATE_MANAGEMENT_H
#define MANDEYE_MULTISENSOR_STATE_MANAGEMENT_H

#include "clients/FileSystemClient.h"
#include "clients/GnssClient.h"
#include "clients/GpioClient.h"
#include "clients/LivoxClient.h"
#include "save_laz.h"
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

std::atomic<bool> isRunning{true};
std::mutex livoxClientPtrLock;
std::shared_ptr<LivoxClient> livoxCLientPtr;
std::shared_ptr<GNSSClient> gnssClientPtr;
std::mutex gpioClientPtrLock;
std::shared_ptr<GpioClient> gpioClientPtr;
std::shared_ptr<FileSystemClient> fileSystemClientPtr;

mandeye::States app_state{mandeye::States::WAIT_FOR_RESOURCES};

std::string produceReport();
bool StartScan();
bool StopScan();

bool TriggerStopScan();
bool TriggerContinousScanning();
void savePointcloudData(LivoxPointsBufferPtr buffer, const std::string& directory, int chunk);
void saveLidarList(const std::unordered_map<uint32_t, std::string> &lidars, const std::string& directory, int chunk);
void saveImuData(LivoxIMUBufferPtr buffer, const std::string& directory, int chunk);
void saveGnssData(std::deque<std::string>& buffer, const std::string& directory, int chunk);
bool saveChunkToDisk(std::string outDirectory, int chunk);

void stateWatcher();
} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_STATE_MANAGEMENT_H
