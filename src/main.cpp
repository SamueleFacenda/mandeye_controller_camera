#include "clients/concrete/FileSystemClient.h"
#include "clients/concrete/GnssClient.h"
#include "clients/concrete/GpioClient.h"
#include "clients/concrete/LivoxClient.h"
#include "clients/concrete/SystemTimeStampProvider.h"
#include "livox_types.h"
#include "state_management.h"
#include "utils/utils.h"
#include "web/ServerHandler.h"
#include <chrono>
#include <iostream>
#include <ostream>
#include <string>
#include <thread>
#include <csignal>
#define MANDEYE_LIVOX_LISTEN_IP "192.168.1.5"
#define MANDEYE_REPO "/media/usb/"
#define MANDEYE_GPIO_SIM false
#define SERVER_PORT 8003
#define MANDEYE_GNSS_UART "/dev/ttyS0"
#define IGNORE_LIDAR_ERROR false

using namespace mandeye;

void initializeCameraClientThread(ThreadMap& threads) {
	std::shared_ptr<CamerasClient> camerasClientPtr = std::make_shared<CamerasClient>(
		utils::getEnvString("MANDEYE_REPO", MANDEYE_REPO),
		threads);
	camerasClientPtr->SetTimeStampProvider(timeStampProviderPtr);
	threads["Cameras Client"] = std::make_shared<std::thread>([=]() {
		camerasClientPtr->receiveImages();
	});
	std::unique_lock<std::shared_mutex> lock(clientsMutex);
	saveableClients.push_back(camerasClientPtr);
	loggerClients.push_back(camerasClientPtr);

	std::cout << "Cameras Client initialized" << std::endl;
}

void initializeStateMachineThread(ThreadMap& threads) {
	threads["State Machine"] = std::make_shared<std::thread>([&]() { stateWatcher(); });
	std::cout << "State Machine initialized" << std::endl;
}

void initializeLivoxClient(bool& lidar_error)
{
	std::shared_ptr<LivoxClient> livoxClientPtr = std::make_shared<LivoxClient>();
	if(!livoxClientPtr->startListener(utils::getEnvString("MANDEYE_LIVOX_LISTEN_IP", MANDEYE_LIVOX_LISTEN_IP))){
		lidar_error = true;
		if (utils::getEnvBool("IGNORE_LIDAR_ERROR", IGNORE_LIDAR_ERROR)) {
			std::cerr << "Ignoring lidar error" << std::endl;
			lidar_error = false;
			timeStampProviderPtr = std::make_shared<SystemTimeStampProvider>();
			return;
		}
	}

	timeStampProviderPtr = livoxClientPtr;

	std::unique_lock<std::shared_mutex> lock(clientsMutex);
	saveableClients.push_back(std::dynamic_pointer_cast<SaveChunkToDirClient>(livoxClientPtr));
	loggerClients.push_back(std::dynamic_pointer_cast<LoggerClient>(livoxClientPtr));
	jsonReportProducerClients.push_back(std::dynamic_pointer_cast<JsonStateProducer>(livoxClientPtr));
	std::cout << "Livox initialized" << std::endl;
}

void initializeGnssClient() {
	const std::string portName = utils::getEnvString("MANDEYE_GNSS_UART", MANDEYE_GNSS_UART);
	if (!portName.empty()) {
		std::cout << "Initialize gnss" << std::endl;
		std::shared_ptr<GNSSClient> gnssClientPtr = std::make_shared<GNSSClient>();
		gnssClientPtr->SetTimeStampProvider(std::dynamic_pointer_cast<TimeStampProvider>(timeStampProviderPtr));
		gnssClientPtr->startListener(portName, 9600);

		std::unique_lock<std::shared_mutex> lock(clientsMutex);
		saveableClients.push_back(gnssClientPtr);
		loggerClients.push_back(gnssClientPtr);
		jsonReportProducerClients.push_back(gnssClientPtr);
	}
	std::cout << "GNSS initialized" << std::endl;
}

void initializeFileSystemClient() {
	fileSystemClientPtr = std::make_shared<FileSystemClient>(utils::getEnvString("MANDEYE_REPO", MANDEYE_REPO));
	std::unique_lock<std::shared_mutex> lock(clientsMutex);
	jsonReportProducerClients.push_back(fileSystemClientPtr);
	std::cout << "FileSystemClient initialized" << std::endl;
}

void initializeGpioClientThread(ThreadMap& threads) {
	using namespace std::chrono_literals;
	const bool simMode = utils::getEnvBool("MANDEYE_GPIO_SIM", MANDEYE_GPIO_SIM);
	std::cout << "MANDEYE_GPIO_SIM : " << simMode << std::endl;
	gpioClientPtr = std::make_shared<GpioClient>(simMode);

	gpioClientPtr->addButtonCallback(BUTTON::BUTTON_STOP_SCAN, "BUTTON_STOP_SCAN", [&]() { TriggerStopScan(); });
	gpioClientPtr->addButtonCallback(BUTTON::BUTTON_CONTINOUS_SCANNING, "BUTTON_CONTINOUS_SCANNING", [&]() { TriggerContinousScanning(); });

	// LED dance signalizing that GPIO is ready
	threads["Gpio"] = std::make_shared<std::thread>([&]() {
		for(int i = 0; i < 3; i++)
		{
			utils::blinkLed(LED::LED_GPIO_STOP_SCAN, 100ms);
			utils::blinkLed(LED::LED_GPIO_CONTINOUS_SCANNING, 100ms);
			utils::blinkLed(LED::LED_GPIO_COPY_DATA, 100ms);
		}
		std::cout << "GPIO Init done" << std::endl;

		std::unique_lock<std::shared_mutex> lock(clientsMutex);
		jsonReportProducerClients.push_back(gpioClientPtr);
		initializationLatch--;
	});
}

void initializePistacheServerThread(ThreadMap& threads, std::shared_ptr<Pistache::Http::Endpoint>& server) {
	using namespace Pistache;

	Address addr(Ipv4::any(), SERVER_PORT);
	server = std::make_shared<Http::Endpoint>(addr);

	threads["Pistache server"] = std::make_shared<std::thread>([&]() {
		auto opts = Http::Endpoint::options().threads(2);
		server->init(opts);
		server->setHandler(Http::make_handler<PistacheServerHandler>());
		std::cout << "Starting Pistache server on port " << SERVER_PORT << std::endl;
		server->serve();
	});
}

void stopApplication(int signal) {
	std::cout << "Stopping application" << std::endl;
	isRunning.store(false);
}

int main(int argc, char** argv)
{
	std::cin.sync_with_stdio(false);
	std::cout << "program: " << argv[0] << " v0.4" << std::endl;
	bool lidar_error = false;
	ThreadMap threadsWithNames;
	std::shared_ptr<Pistache::Http::Endpoint> server;

	initializePistacheServerThread(threadsWithNames, server);
	initializeFileSystemClient();
	initializeLivoxClient(lidar_error);
	initializeGnssClient();
	initializeStateMachineThread(threadsWithNames);
	initializeGpioClientThread(threadsWithNames);
	initializeCameraClientThread(threadsWithNames);

	signal(SIGINT, stopApplication);

	// Main cycle (cli interface)
	using namespace std::chrono_literals;
	char ch = ' ';
	std::cout << "Press q -> quit, s -> start scan , e -> end scan" << std::endl;
	do {
		if (lidar_error) { // loop guard clause
			app_state = States::LIDAR_ERROR;
			std::cout << "lidar error" << std::endl;
			std::this_thread::sleep_for(1000ms);
			continue; // skip the rest
		}

		// check if there is any input, don't do blocking read
		if (std::cin.rdbuf()->in_avail() == 0) {
			std::this_thread::sleep_for(100ms);
			continue;
		}

		std::cin.get(ch);
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
			isRunning.store(false);
			std::cout << "Stopping..." << std::endl;
			break;
		case '\n':
			break;
		default:
			std::cerr << "Unknown instruction: '" << ch << "'" << std::endl;
		}
	} while (isRunning.load());

	//// Stop and join everyone

	server->shutdown(); // http server stop

	for(const auto& [name, thread]: threadsWithNames) {
		std::cout << "joining " << name << " thread" << std::endl;
		thread->join();
	}

	std::cout << "Done" << std::endl;
	return 0;
}
