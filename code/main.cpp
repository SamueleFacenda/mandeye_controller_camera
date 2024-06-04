
#include <chrono>
#include <json.hpp>
#include <ostream>
#include <thread>

#include "clients/FileSystemClient.h"
#include "clients/GnssClient.h"
#include "clients/GpioClient.h"
#include "clients/LivoxClient.h"
#include "web/ServerHandler.h"
#include "utils/utils.h"
#include "state_management.h"
#include <iostream>
#include <string>

//configuration for alienware
#define MANDEYE_LIVOX_LISTEN_IP "192.168.1.5"
#define MANDEYE_REPO "/media/usb/"
#define MANDEYE_GPIO_SIM false
#define SERVER_PORT 8003
#define MANDEYE_GNSS_UART "/dev/ttyS0"

int main(int argc, char** argv)
{
	std::cout << "program: " << argv[0] << " v0.4" << std::endl;
	bool lidar_error = false;
	using namespace Pistache;

	Address addr(Ipv4::any(), SERVER_PORT);

	auto server = std::make_shared<Http::Endpoint>(addr);
	std::thread http_thread1([&]() {
		auto opts = Http::Endpoint::options().threads(2);
		server->init(opts);
		server->setHandler(Http::make_handler<PistacheServerHandler>());
		server->serve();
	});

	mandeye::fileSystemClientPtr = std::make_shared<mandeye::FileSystemClient>(utils::getEnvString("MANDEYE_REPO", MANDEYE_REPO));

	std::thread thLivox([&]() {
		{
			std::lock_guard<std::mutex> l1(mandeye::livoxClientPtrLock);
			mandeye::livoxClientPtr = std::make_shared<mandeye::LivoxClient>();
		}
		if(!mandeye::livoxClientPtr->startListener(utils::getEnvString("MANDEYE_LIVOX_LISTEN_IP", MANDEYE_LIVOX_LISTEN_IP))){
			lidar_error = true;
		}

		// initialize in this thread to prevent initialization fiasco
        const std::string portName = utils::getEnvString("MANDEYE_GNSS_UART", MANDEYE_GNSS_UART);
        if (!portName.empty())
        {
            mandeye::gnssClientPtr = std::make_shared<mandeye::GNSSClient>();
            mandeye::gnssClientPtr->SetTimeStampProvider(mandeye::livoxClientPtr);
            mandeye::gnssClientPtr->startListener(portName, 9600);
        }
	});
	

	std::thread thStateMachine([&]() { mandeye::stateWatcher(); });

	std::thread thGpio([&]() {
		std::lock_guard<std::mutex> l2(mandeye::gpioClientPtrLock);
		using namespace std::chrono_literals;
		const bool simMode = utils::getEnvBool("MANDEYE_GPIO_SIM", MANDEYE_GPIO_SIM);
		std::cout << "MANDEYE_GPIO_SIM : " << simMode << std::endl;
		mandeye::gpioClientPtr = std::make_shared<mandeye::GpioClient>(simMode);
		for(int i = 0; i < 3; i++)
		{
			utils::blinkLed(mandeye::GpioClient::LED::LED_GPIO_STOP_SCAN, 100ms);
			utils::blinkLed(mandeye::GpioClient::LED::LED_GPIO_CONTINOUS_SCANNING, 100ms);
			utils::blinkLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, 100ms);
		}
		std::cout << "GPIO Init done" << std::endl;
		mandeye::gpioClientPtr->addButtonCallback(mandeye::GpioClient::BUTTON::BUTTON_STOP_SCAN, "BUTTON_STOP_SCAN", [&]() { mandeye::TriggerStopScan(); });
		mandeye::gpioClientPtr->addButtonCallback(mandeye::GpioClient::BUTTON::BUTTON_CONTINOUS_SCANNING, "BUTTON_CONTINOUS_SCANNING", [&]() { mandeye::TriggerContinousScanning(); });
	});

	while(mandeye::isRunning)
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(1000ms);
		char ch = std::getchar();
		if(ch == 'q')
		{
			mandeye::isRunning.store(false);
		}
		if(!lidar_error){
			std::cout << "Press q -> quit, s -> start scan , e -> end scan" << std::endl;

			if(ch == 's')
			{
				if(mandeye::StartScan())
				{
					std::cout << "start scan success!" << std::endl;
				}
			}
			else if(ch == 'e')
			{
				if(mandeye::StopScan())
				{
					std::cout << "stop scan success!" << std::endl;
				}
			}
		}else{
			mandeye::app_state = mandeye::States::LIDAR_ERROR;
			std::cout << "lidar error" << std::endl;
			std::this_thread::sleep_for(1000ms);
		}
	}

	server->shutdown();
	http_thread1.join();
	std::cout << "joining thStateMachine" << std::endl;
	thStateMachine.join();

	std::cout << "joining thLivox" << std::endl;
	thLivox.join();

	std::cout << "joining thGpio" << std::endl;
	thGpio.join();
	std::cout << "Done" << std::endl;
	return 0;
}
