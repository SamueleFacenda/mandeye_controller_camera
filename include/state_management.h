#ifndef MANDEYE_MULTISENSOR_STATE_MANAGEMENT_H
#define MANDEYE_MULTISENSOR_STATE_MANAGEMENT_H

#include "clients/concrete/CamerasClient.h"
#include "clients/concrete/FileSystemClient.h"
#include "clients/concrete/GnssClient.h"
#include "clients/concrete/GpioClient.h"
#include "clients/concrete/LivoxClient.h"
#include "utils/save_laz.h"
#include <iostream>
#include <shared_mutex>
#include <atomic>
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
	STARTING_STOP_SCAN = 100,
	STOP_SCAN_IN_PROGRESS = 150,
	STOP_SCAN_IN_INITIAL_PROGRESS = 160,
	STOPPING_STOP_SCAN = 190,
	LIDAR_ERROR = 200,
	USB_IO_ERROR = 210,
};

const std::map<States, std::string> StatesToString{
	{States::WAIT_FOR_RESOURCES, "WAIT_FOR_RESOURCES"},
	{States::IDLE, "IDLE"},
	{States::STARTING_SCAN, "STARTING_SCAN"},
	{States::SCANNING, "SCANNING"},
	{States::STOPPING, "STOPPING"},
	{States::STARTING_STOP_SCAN, "STARTING_STOP_SCAN"},
	{States::STOP_SCAN_IN_PROGRESS, "STOP_SCAN_IN_PROGRESS"},
	{States::STOP_SCAN_IN_INITIAL_PROGRESS, "STOP_SCAN_IN_INITIAL_PROGRESS"},
	{States::STOPPING_STOP_SCAN, "STOPPING_STOP_SCAN"},
	{States::LIDAR_ERROR, "LIDAR_ERROR"},
	{States::USB_IO_ERROR, "USB_IO_ERROR"},
};

extern std::atomic<bool> isRunning;
extern std::shared_ptr<LivoxClient> livoxClientPtr;
extern std::shared_ptr<GpioClient> gpioClientPtr;
extern std::shared_ptr<FileSystemClient> fileSystemClientPtr;
extern std::vector<std::shared_ptr<SaveChunkToDirClient>> saveableClients;
extern std::vector<std::shared_ptr<LoggerClient>> loggerClients;
extern std::vector<std::shared_ptr<JsonStateProducer>> jsonReportProducerClients;
extern std::shared_mutex clientsMutex; // only used in initialization
extern std::shared_lock<std::shared_mutex> clientsReadLock; // can be used when accessing clients lists, but they are safe now
extern std::unique_lock<std::shared_mutex> clientsWriteLock;
extern std::atomic<int> initializationLatch;

extern States app_state;

std::string produceReport();
bool StartScan();
bool StopScan();

bool TriggerStopScan();
bool TriggerContinousScanning();
bool saveChunkToDisk(const std::string& outDirectory, int chunk);

void stateWatcher();
} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_STATE_MANAGEMENT_H
