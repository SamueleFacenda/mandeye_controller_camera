#include "clients/concrete/GpioClient.h"
#include <iostream>

#define DEBOUNCE_TIME 10000

namespace mandeye
{

GpioClient::GpioClient(bool sim)
	: m_useSimulatedGPIO(sim)
{

	for(auto& [id, name] : ButtonToName)
		m_buttonsCallbacks[id] = Callbacks{};

	if(!sim) {
		if (gpioInitialise() == PI_INIT_FAILED) {
			std::cout << "ERROR: Failed to initialize the GPIO interface." << std::endl;
			return;
		}

		std::lock_guard<std::mutex> lck{m_lock};

		for(auto& [led, state] : m_ledState)
			initLed(led);

		for(auto& [button, state] : m_buttonState)
			initButton(button);
	}

	for(auto& [buttonID, ButtonName] : ButtonToName)
		addButtonCallback(buttonID, "DBG" + ButtonName, [&]() {
			std::cout << "TestButton " << ButtonName << std::endl;
		});
}

GpioClient::~GpioClient() {
	if(!m_useSimulatedGPIO)
		gpioTerminate();
}


nlohmann::json GpioClient::produceStatus()
{
	nlohmann::json data;
	std::lock_guard<std::mutex> lck{m_lock};
	for(const auto& [ledid, ledname] : LedToName)
	{
		try
		{
			data["leds"][ledname] = m_ledState.at(ledid);
		}
		catch(const std::out_of_range& ex)
		{
			data["leds"][ledname] = "NA";
		}
	}

	for(const auto& [buttonid, buttonname] : ButtonToName)
	{
		try
		{
			data["buttons"][buttonname] = m_buttonState.at(buttonid);
		}
		catch(const std::out_of_range& ex)
		{
			data["buttons"][buttonname] = "NA";
		}
	}
	return data;
}

void GpioClient::setLed(LED led, bool state)
{
	std::lock_guard<std::mutex> lck{m_lock};
	m_ledState[led] = state;
	if(!m_useSimulatedGPIO)
		gpioWrite((int) led, state);
}

void GpioClient::addButtonCallback(BUTTON btn,
								   const std::string& callbackName,
								   const std::function<void()>& callback)
{
	std::lock_guard<std::mutex> lck{m_lock};
	// check if button exists (do I really need this? It's not checked anywhere else)
	auto it = m_buttonsCallbacks.find(btn);
	if(it == m_buttonsCallbacks.end())
	{
		std::cerr << "No button with id " << (int)btn << std::endl;
		return;
	}
	(it->second)[callbackName] = callback;
}

std::string GpioClient::getJsonName()
{
	return "gpio";
}

void GpioClient::initLed(LED led) {
	gpioSetMode((int) led, PI_OUTPUT);
	gpioWrite((int) led, PI_LOW);
	m_ledState[led] = false;
}

void GpioClient::btnCallback(int gpio, int level, uint32_t tick, void* userdata) {
	auto* thiss = (GpioClient*) userdata;
	auto btn = (BUTTON) gpio;
	if (tick - thiss->m_buttonLastPressTime[btn] < DEBOUNCE_TIME)
		return;

	thiss->m_buttonLastPressTime[btn] = tick;
	thiss->m_buttonState[btn] = level == PI_LOW;
	if (thiss->m_buttonState[btn])
	{
		for(auto& [name, callback] : thiss->m_buttonsCallbacks[btn])
			callback();
	}
}

void GpioClient::initButton(BUTTON btn) {
	gpioSetMode((int) btn, PI_INPUT);
	gpioSetPullUpDown((int) btn, PI_PUD_UP);
	gpioSetAlertFuncEx((int) btn, btnCallback, this);
	m_buttonState[btn] = false;
}

} // namespace mandeye