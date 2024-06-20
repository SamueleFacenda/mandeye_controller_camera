#include "state_management.h"
#include "clients/concrete/FileSystemClient.h"
#include "clients/concrete/GpioClient.h"
#include "clients/concrete/LivoxClient.h"
#include "utils/utils.h"
#include <iostream>
#include <string>
#include <execution>

// in seconds
#define STOP_SCAN_DURATION 5s
#define CONTINOUS_SCAN_SAVE_INTERVAL 10s

namespace mandeye
{

std::atomic<bool> isRunning{true};
std::shared_ptr<LivoxClient> livoxClientPtr;
std::shared_ptr<GpioClient> gpioClientPtr;
std::shared_ptr<FileSystemClient> fileSystemClientPtr;
States app_state{States::WAIT_FOR_RESOURCES};
std::vector<std::shared_ptr<SaveChunkToDirClient>> saveableClients;
std::vector<std::shared_ptr<LoggerClient>> loggerClients;
std::vector<std::shared_ptr<JsonStateProducer>> jsonReportProducerClients;
std::shared_mutex clientsMutex; // only used in initialization
std::atomic<int> initializationLatch{3}; // there are `n` initialization steps: gpio client, livox client + gnss, camera client


std::string produceReport()
{
	using json = nlohmann::json;
	json j;
	j["name"] = "Mandye";
	j["state"] = StatesToString.at(app_state);
	j["livox"] = {};
	j["gnss"] = {};

	for (auto& client : jsonReportProducerClients)
		j[client->getJsonName()] = client->produceStatus();

	std::ostringstream s;
	s << std::setw(4) << j;
	return s.str();
}

bool StartScan()
{
	if(app_state == States::IDLE)
	{
		app_state = States::STARTING_SCAN;
		return true;
	}
	return false;
}
bool StopScan()
{
	if(app_state == States::SCANNING)
	{
		app_state = States::STOPPING;
		return true;
	}
	return false;
}

bool TriggerStopScan()
{
	if(app_state == States::IDLE)
	{
		app_state = States::STARTING_STOP_SCAN;
		return true;
	}
	return false;
}

bool TriggerContinousScanning(){
	if(app_state == States::IDLE){
		app_state = States::STARTING_SCAN;
		return true;
	}else if(app_state == States::SCANNING)
	{
		app_state = States::STOPPING;
		return true;
	}
	return false;
}

bool saveChunkToDisk(const std::string& outDirectory, int chunk)
{
	if(outDirectory.empty())
	{
		app_state = States::USB_IO_ERROR;
		return false;
	}
	gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_COPY_DATA, true);

	// parallelize the saving of the chunks
	std::for_each(std::execution::par_unseq, saveableClients.begin(), saveableClients.end(), [&outDirectory, chunk](auto& client){
		client->saveChunkToDirectory(outDirectory, chunk);
	});

	utils::syncDisk();
	gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_COPY_DATA, false);
	return true;
}

using namespace std::chrono_literals;

void stateWatcher()
{
	std::chrono::steady_clock::time_point chunkStart = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point stopScanDeadline = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point stopScanInitialDeadline = std::chrono::steady_clock::now();
	States oldState = States::IDLE;
	std::string continousScanDirectory;
	std::string stopScanDirectory;
	int chunksInExperimentCS{0};
	int chunksInExperimentSS{0};

	int id_manifest = 0;
	if (stopScanDirectory.empty() && fileSystemClientPtr)
	{
		if(!fileSystemClientPtr->CreateDirectoryForStopScans(stopScanDirectory, id_manifest)){
			app_state = States::USB_IO_ERROR;
		}
	}
	if(stopScanDirectory.empty()){
		app_state = States::USB_IO_ERROR;
	}

	if(!fileSystemClientPtr->CreateDirectoryForContinousScanning(continousScanDirectory, id_manifest)){
		app_state = States::USB_IO_ERROR;
	}

	while(isRunning.load())
	{
		if(oldState != app_state)
		{
			std::cout << "State transition from " << StatesToString.at(oldState) << " to " << StatesToString.at(app_state) << std::endl;
		}
		oldState = app_state;

		if(app_state == States::LIDAR_ERROR){
			if(gpioClientPtr){
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, false);
				std::this_thread::sleep_for(1000ms);
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_STOP_SCAN, true);
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, true);
				std::this_thread::sleep_for(1000ms);
			}
			std::cout << "app_state == States::LIDAR_ERROR" << std::endl;
		}
		else if(app_state == States::USB_IO_ERROR){
			if(gpioClientPtr){
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, false);
				utils::blinkLed(GpioClient::LED::LED_GPIO_COPY_DATA, 1s);
			}
			std::cout << "app_state == States::USB_IO_ERROR" << std::endl;
		}
		else if(app_state == States::WAIT_FOR_RESOURCES)
		{
			std::this_thread::sleep_for(100ms);
			if (initializationLatch.load() == 0)
			{
				app_state = States::IDLE;
			}
		}
		else if(app_state == States::IDLE)
		{
			if(gpioClientPtr)
			{
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, false);
			}
			std::this_thread::sleep_for(100ms);
		}
		else if(app_state == States::STARTING_SCAN)
		{
			if(gpioClientPtr)
			{
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				utils::blinkLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 100ms);
				utils::blinkLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 100ms);
			}

			for(auto& client: loggerClients)
				client->startLog();

			app_state = States::SCANNING;
			chunkStart = std::chrono::steady_clock::now();
		}
		else if(app_state == States::SCANNING)
		{
			if(gpioClientPtr)
			{
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, true);
			}

			const auto now = std::chrono::steady_clock::now();
			if(now - chunkStart > 60s)
			{
				utils::blinkLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 1000ms);
			}
			if(now - chunkStart > 600s)
			{
				utils::blinkLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 100ms);
			}
			if(now - chunkStart > CONTINOUS_SCAN_SAVE_INTERVAL)
			{
				chunkStart = std::chrono::steady_clock::now();

				bool savingDone = saveChunkToDisk(continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
				if (savingDone)
				{
					chunksInExperimentCS++;
				}
			}
			std::this_thread::sleep_for(100ms);
		}
		else if(app_state == States::STOPPING)
		{
			gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, true);
			bool savingDone = saveChunkToDisk(continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
			if (savingDone)
			{
				chunksInExperimentCS++;
				app_state = States::IDLE;

				for(auto& client: loggerClients)
					client->stopLog();
			}
		}
		else if(app_state == States::STARTING_STOP_SCAN)
		{
			if(gpioClientPtr)
			{
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, false);
			}

			stopScanInitialDeadline = std::chrono::steady_clock::now() + 10s; // wait 10 seconds before starting

			stopScanDeadline = stopScanInitialDeadline + STOP_SCAN_DURATION;

			app_state = States::STOP_SCAN_IN_INITIAL_PROGRESS;
		}
		else if(app_state == States::STOP_SCAN_IN_INITIAL_PROGRESS){
			const auto now = std::chrono::steady_clock::now();

			if(now < stopScanInitialDeadline){
				utils::blinkLed(GpioClient::LED::LED_GPIO_STOP_SCAN, 100ms);
			}else{
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_STOP_SCAN, true);
				for(auto& client: loggerClients)
					client->startLog();

				app_state = States::STOP_SCAN_IN_PROGRESS;
			}
		}
		else if(app_state == States::STOP_SCAN_IN_PROGRESS)
		{
			const auto now = std::chrono::steady_clock::now();
			if(now > stopScanDeadline){
				app_state = States::STOPPING_STOP_SCAN;
			}
		}
		else if(app_state == States::STOPPING_STOP_SCAN)
		{
			bool savingDone = saveChunkToDisk(stopScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
			if (savingDone)
			{
				for(auto& client: loggerClients)
					client->stopLog();

				chunksInExperimentSS++;
				app_state = States::IDLE;
				gpioClientPtr->setLed(GpioClient::LED::LED_GPIO_STOP_SCAN, false);
			}
		}
	}
}
} // namespace mandeye