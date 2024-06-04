#include "state_management.h"
#include "clients/FileSystemClient.h"
#include "clients/GnssClient.h"
#include "clients/GpioClient.h"
#include "clients/LivoxClient.h"
#include "save_laz.h"
#include <fstream>
#include <iostream>
#include <string>

namespace mandeye
{

using json = nlohmann::json;

std::string produceReport()
{
	json j;
	j["name"] = "Mandye";
	j["state"] = StatesToString.at(app_state);
	if(livoxCLientPtr)
	{
		j["livox"] = livoxCLientPtr->produceStatus();
	}
	else
	{
		j["livox"] = {};
	}

	if(gpioClientPtr)
	{
		j["gpio"] = gpioClientPtr->produceStatus();
	}

	if(fileSystemClientPtr)
	{
		j["fs"] = fileSystemClientPtr->produceStatus();
	}
	if(gnssClientPtr)
	{
		j["gnss"] = gnssClientPtr->produceStatus();
	}else{
		j["gnss"] = {};
	}

	std::ostringstream s;
	s << std::setw(4) << j;
	return s.str();
}

bool StartScan()
{
	if(app_state == States::IDLE || app_state == States::STOPPED)
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

using namespace std::chrono_literals;

bool TriggerStopScan()
{
	if(app_state == States::IDLE || app_state == States::STOPPED)
	{
		app_state = States::STARTING_STOP_SCAN;
		return true;
	}
	return false;
}

bool TriggerContinousScanning(){
	if(app_state == States::IDLE || app_state == States::STOPPED){
		app_state = States::STARTING_SCAN;
		return true;
	}else if(app_state == States::SCANNING)
	{
		app_state = States::STOPPING;
		return true;
	}
	return false;
}

void savePointcloudData(LivoxPointsBufferPtr buffer, const std::string& directory, int chunk)
{
	using namespace std::chrono_literals;
	char lidarName[256];
	snprintf(lidarName, 256, "lidar%04d.laz", chunk);
	std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(lidarName);
	std::cout << "Savig lidar buffer of size " << buffer->size() << " to " << lidarFilePath << std::endl;
	saveLaz(lidarFilePath.string(), buffer);
	//        std::ofstream lidarStream(lidarFilePath.c_str());
	//        for (const auto &p : *buffer){
	//            lidarStream<<p.point.x << " "<<p.point.y <<"  "<<p.point.z << " "<< p.point.tag << " " << p.timestamp << "\n";
	//        }
	system("sync");
	return;
}

void saveLidarList(const std::unordered_map<uint32_t, std::string> &lidars, const std::string& directory, int chunk)
{
	using namespace std::chrono_literals;
	char lidarName[256];
	snprintf(lidarName, 256, "lidar%04d.sn", chunk);
	std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(lidarName);
	std::cout << "Savig lidar list of size " << lidars.size() << " to " << lidarFilePath << std::endl;

	std::ofstream lidarStream(lidarFilePath);
	for (const auto &[id,sn] : lidars){
		lidarStream << id << " " << sn << "\n";
	}
	system("sync");
	return;
}

void saveImuData(LivoxIMUBufferPtr buffer, const std::string& directory, int chunk)
{
	using namespace std::chrono_literals;
	char lidarName[256];
	snprintf(lidarName, 256, "imu%04d.csv", chunk);
	std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(lidarName);
	std::cout << "Savig imu buffer of size " << buffer->size() << " to " << lidarFilePath << std::endl;
	std::ofstream lidarStream(lidarFilePath.c_str());
	std::stringstream ss;

	for(const auto& p : *buffer)
	{
		if(p.timestamp > 0){
			ss << p.timestamp << " " << p.point.gyro_x << " " << p.point.gyro_y << " " << p.point.gyro_z << " " << p.point.acc_x << " "
			   << p.point.acc_y << " " << p.point.acc_z << " " << p.laser_id << "\n";
		}
	}
	lidarStream << ss.rdbuf();

	lidarStream.close();
	system("sync");
	return;
}

void saveGnssData(std::deque<std::string>& buffer, const std::string& directory, int chunk)
{
	if(buffer.size() == 0 && mandeye::gnssClientPtr){
		mandeye::gnssClientPtr->startListener(utils::getEnvString("MANDEYE_GNSS_UART", MANDEYE_GNSS_UART), 9600);
	}
	using namespace std::chrono_literals;
	char lidarName[256];
	snprintf(lidarName, 256, "gnss%04d.gnss", chunk);
	std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(lidarName);
	std::cout << "Savig gnss buffer of size " << buffer.size() << " to " << lidarFilePath << std::endl;
	std::ofstream lidarStream(lidarFilePath.c_str());
	std::stringstream ss;

	for(const auto& p : buffer)
	{
		ss << p ;
	}
	lidarStream << ss.rdbuf();

	lidarStream.close();
	system("sync");
	return;
}

bool saveChunkToDisk(std::string outDirectory, int chunk) {
	mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, true);

	auto [lidarBuffer, imuBuffer] = livoxCLientPtr->retrieveData();
	std::deque<std::string> gnssBuffer;
	if (gnssClientPtr)
	{
		gnssBuffer = gnssClientPtr->retrieveData();
	}

	if(outDirectory == ""){
		app_state = States::USB_IO_ERROR;
		return false;
	}

	savePointcloudData(lidarBuffer, outDirectory, chunk);
	saveImuData(imuBuffer, outDirectory, chunk);
	auto lidarList = livoxCLientPtr->getSerialNumberToLidarIdMapping();
	saveLidarList(lidarList, outDirectory, chunk);
	if (gnssClientPtr)
	{
		saveGnssData(gnssBuffer, outDirectory, chunk);
		saveGnssData(gnssBuffer, outDirectory, chunk);
	}
	mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, false);

	return true;
}

void stateWatcher()
{
	using namespace std::chrono_literals;
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


	while(isRunning)
	{
		if(oldState != app_state)
		{
			std::cout << "State transtion from " << StatesToString.at(oldState) << " to " << StatesToString.at(app_state) << std::endl;
		}
		oldState = app_state;

		if(app_state == States::LIDAR_ERROR){
			if(mandeye::gpioClientPtr){
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, false);
				std::this_thread::sleep_for(1000ms);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, true);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, true);
				std::this_thread::sleep_for(1000ms);
				std::cout << "app_state == States::LIDAR_ERROR" << std::endl;
			}
		}if(app_state == States::USB_IO_ERROR){
			if(mandeye::gpioClientPtr){
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, false);
				std::this_thread::sleep_for(1000ms);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, true);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, false);
				std::this_thread::sleep_for(1000ms);
				std::cout << "app_state == States::USB_IO_ERROR" << std::endl;
			}
		}
		else if(app_state == States::WAIT_FOR_RESOURCES)
		{
			std::this_thread::sleep_for(100ms);
			std::lock_guard<std::mutex> l1(livoxClientPtrLock);
			std::lock_guard<std::mutex> l2(gpioClientPtrLock);
			if(mandeye::gpioClientPtr && mandeye::fileSystemClientPtr)
			{
				app_state = States::IDLE;
			}
		}
		else if(app_state == States::IDLE)
		{
			if(gpioClientPtr)
			{
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, false);
			}
			std::this_thread::sleep_for(100ms);
		}
		else if(app_state == States::STARTING_SCAN)
		{
			if(gpioClientPtr)
			{
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, false);
				utils::blinkLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 100ms);
				utils::blinkLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 100ms);
			}

			if(livoxCLientPtr)
			{
				livoxCLientPtr->startLog();
				if (gnssClientPtr)
				{
					gnssClientPtr->startLog();
				}
				app_state = States::SCANNING;
			}
			// create directory
			//if(!fileSystemClientPtr->CreateDirectoryForExperiment(continousScanDirectory)){
			//	app_state = States::USB_IO_ERROR;
			//}
			chunkStart = std::chrono::steady_clock::now();
		}
		else if(app_state == States::SCANNING)
		{
			if(gpioClientPtr)
			{
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, true);
			}

			const auto now = std::chrono::steady_clock::now();
			if(now - chunkStart > std::chrono::seconds(60))
			{
				utils::blinkLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 1000ms);
			}
			if(now - chunkStart > std::chrono::seconds(600))
			{
				utils::blinkLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 100ms);
			}
			if(now - chunkStart > std::chrono::seconds(10))
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
			mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, true);
			bool savingDone = saveChunkToDisk(continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
			if (savingDone)
			{
				chunksInExperimentCS++;
				app_state = States::IDLE;
			}
		}
		else if(app_state == States::STARTING_STOP_SCAN)
		{
			if(gpioClientPtr)
			{
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, false);
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, false);
			}

			stopScanInitialDeadline = std::chrono::steady_clock::now();
			stopScanInitialDeadline += 5000ms;

			stopScanDeadline = stopScanInitialDeadline + 30000ms;

			app_state = States::STOP_SCAN_IN_INITIAL_PROGRESS;
		}else if(app_state == States::STOP_SCAN_IN_INITIAL_PROGRESS){
			const auto now = std::chrono::steady_clock::now();

			if(now < stopScanInitialDeadline){
				utils::blinkLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, 100ms);
			}else{
				if(livoxCLientPtr)
				{
					mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, true);
					livoxCLientPtr->startLog();
				}
				if (gnssClientPtr)
				{
					gnssClientPtr->startLog();
				}
				app_state = States::STOP_SCAN_IN_PROGRESS;
			}
		}
		else if(app_state == States::STOP_SCAN_IN_PROGRESS)
		{
			const auto now = std::chrono::steady_clock::now();
			if(now > stopScanDeadline){
				app_state = States::STOPING_STOP_SCAN;
			}
		}
		else if(app_state == States::STOPING_STOP_SCAN)
		{
			bool savingDone = saveChunkToDisk(stopScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
			if (savingDone)
			{
				chunksInExperimentSS++;
				app_state = States::IDLE;
				mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, false);
			}
		}
	}
}
} // namespace mandeye