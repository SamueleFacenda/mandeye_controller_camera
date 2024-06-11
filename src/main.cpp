#include "clients/concrete/FileSystemClient.h"
#include "clients/concrete/GnssClient.h"
#include "clients/concrete/GpioClient.h"
#include "clients/concrete/LivoxClient.h"
#include "state_management.h"
#include "utils/utils.h"
#include "web/ServerHandler.h"
#include <chrono>
#include <iostream>
#include <json.hpp>
#include <ostream>
#include <string>
#include <thread>

//configuration for alienware
#define MANDEYE_LIVOX_LISTEN_IP "192.168.1.5"
#define MANDEYE_REPO "/media/usb/"
#define MANDEYE_GPIO_SIM false
#define SERVER_PORT 8003
#define MANDEYE_GNSS_UART ""
// #define MANDEYE_GNSS_UART "/dev/ttyS0"

using namespace mandeye;

using threadMap = std::unordered_map<std::string,std::shared_ptr<std::thread>>;

void initializeCameraClientThread(threadMap& threads) {
	std::shared_ptr<CamerasClient> camerasClientPtr = std::make_shared<CamerasClient>(utils::getIntListFromEnvVar("MANDEYE_CAMERA_IDS","0"));
	std::shared_ptr<std::thread> thCameras = std::make_shared<std::thread>([&]() {
		camerasClientPtr->receiveImages();
	});
	{
		std::lock_guard<std::unique_lock<std::shared_mutex>> l1(clientsWriteLock);
		saveableClients.push_back(camerasClientPtr);
		loggerClients.push_back(camerasClientPtr);
	}

	threads["Cameras Client"] = thCameras;
	initializationLatch.count_down();
}

void initializeStateMachineThread(threadMap& threads) {
	std::shared_ptr<std::thread> thStateMachine = std::make_shared<std::thread>([&]() { stateWatcher(); });
	threads["State Machine"] = thStateMachine;
}

void initializeLivoxThreadAndGnss(threadMap& threads, std::atomic<bool>& lidar_error) {
	std::shared_ptr<std::thread> thLivox = std::make_shared<std::thread>([&]() {
		livoxClientPtr = std::make_shared<LivoxClient>();
		if(!livoxClientPtr->startListener(utils::getEnvString("MANDEYE_LIVOX_LISTEN_IP", MANDEYE_LIVOX_LISTEN_IP))){
			lidar_error.store(true);
		}

		// initialize gnss client in this thread to prevent initialization fiasco
		const std::string portName = utils::getEnvString("MANDEYE_GNSS_UART", MANDEYE_GNSS_UART);
		std::shared_ptr<GNSSClient> gnssClientPtr;
		if (!portName.empty())
		{
			gnssClientPtr = std::make_shared<GNSSClient>();
			gnssClientPtr->SetTimeStampProvider(std::dynamic_pointer_cast<mandeye_utils::TimeStampProvider>(livoxClientPtr));
			gnssClientPtr->startListener(portName, 9600);
		}

		// acquire write lock and add clients
		std::lock_guard<std::unique_lock<std::shared_mutex>> l1(clientsWriteLock);
		saveableClients.push_back(std::dynamic_pointer_cast<mandeye_utils::SaveChunkToDirClient>(livoxClientPtr));
		loggerClients.push_back(std::dynamic_pointer_cast<mandeye_utils::LoggerClient>(livoxClientPtr));
		jsonReportProducerClients.push_back(std::dynamic_pointer_cast<mandeye_utils::JsonStateProducer>(livoxClientPtr));
		initializationLatch.count_down();

		saveableClients.push_back(gnssClientPtr);
		loggerClients.push_back(gnssClientPtr);
		jsonReportProducerClients.push_back(livoxClientPtr);
		initializationLatch.count_down();
	});
	threads["Livox"] = thLivox;
}

void initializeFileSystemClient() {
	fileSystemClientPtr = std::make_shared<FileSystemClient>(utils::getEnvString("MANDEYE_REPO", MANDEYE_REPO));
	{
		std::lock_guard<std::unique_lock<std::shared_mutex>> l1(clientsWriteLock);
		jsonReportProducerClients.push_back(fileSystemClientPtr);
	}
}

void initializeGpioClientThread(threadMap& threads) {
	using namespace std::chrono_literals;
	std::shared_ptr<std::thread> thGpio = std::make_shared<std::thread>([&]() {
		const bool simMode = utils::getEnvBool("MANDEYE_GPIO_SIM", MANDEYE_GPIO_SIM);
		std::cout << "MANDEYE_GPIO_SIM : " << simMode << std::endl;
		gpioClientPtr = std::make_shared<GpioClient>(simMode);
		for(int i = 0; i < 3; i++)
		{
			utils::blinkLed(GpioClient::LED::LED_GPIO_STOP_SCAN, 100ms);
			utils::blinkLed(GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 100ms);
			utils::blinkLed(GpioClient::LED::LED_GPIO_COPY_DATA, 100ms);
		}
		std::cout << "GPIO Init done" << std::endl;
		gpioClientPtr->addButtonCallback(GpioClient::BUTTON::BUTTON_STOP_SCAN, "BUTTON_STOP_SCAN", [&]() { TriggerStopScan(); });
		gpioClientPtr->addButtonCallback(GpioClient::BUTTON::BUTTON_CONTINOUS_SCANNING, "BUTTON_CONTINOUS_SCANNING", [&]() { TriggerContinousScanning(); });

		std::lock_guard<std::unique_lock<std::shared_mutex>> l1(clientsWriteLock);
		jsonReportProducerClients.push_back(gpioClientPtr);

		initializationLatch.count_down();
	});
	threads["Gpio"] = thGpio;
}

void initializePistacheServerThread(threadMap& threads, std::shared_ptr<Pistache::Http::Endpoint>& server) {
	using namespace Pistache;

	Address addr(Ipv4::any(), SERVER_PORT);
	server = std::make_shared<Http::Endpoint>(addr);

	std::shared_ptr<std::thread> http_thread = std::make_shared<std::thread>([&]() {
		auto opts = Http::Endpoint::options().threads(2);
		server->init(opts);
		server->setHandler(Http::make_handler<PistacheServerHandler>());
		server->serve();
	});
	threads["Pistache server"] = http_thread;
}

int main(int argc, char** argv)
{
	std::cout << "program: " << argv[0] << " v0.4" << std::endl;
	std::atomic<bool> lidar_error = false;
	std::unordered_map<std::string,std::shared_ptr<std::thread>> threadsWithNames;
	std::shared_ptr<Pistache::Http::Endpoint> server;

	initializePistacheServerThread(threadsWithNames, server);
	initializeFileSystemClient();
	initializeLivoxThreadAndGnss(threadsWithNames, lidar_error);
	initializeCameraClientThread(threadsWithNames);
	initializeStateMachineThread(threadsWithNames);
	initializeGpioClientThread(threadsWithNames);

	// Main cycle (cli interface)
	using namespace std::chrono_literals;
	char ch;
	do {
		std::this_thread::sleep_for(1000ms);
		std::cin.get(ch);

		if (lidar_error.load()) { // loop guard clause
			app_state = States::LIDAR_ERROR;
			std::cout << "lidar error" << std::endl;
			std::this_thread::sleep_for(1000ms);
			continue; // skip the rest
		}

		std::cout << "Press q -> quit, s -> start scan , e -> end scan" << std::endl;
		switch(ch) {
		case 's':
			if(StartScan())
			{
				std::cout << "start scan success!" << std::endl;
			}
			break;
		case 'e':
			if(StopScan())
			{
				std::cout << "stop scan success!" << std::endl;
			}
			break;
		case 'q':
			std::cout << "Stopping..." << std::endl;
			break;
		default:
			std::cerr << "Unknown instruction: '" << ch << "'" << std::endl;
		}
	} while (ch != 'q');

	// Stop and join everyone

	isRunning.store(false); // stop state machine and cameras
	server->shutdown(); // http server stop

	for(const auto& [name, thread]: threadsWithNames) {
		std::cout << "joining " << name << " thread" << std::endl;
		thread->join();
	}

	std::cout << "Done" << std::endl;
	return 0;
}
