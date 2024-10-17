#pragma once
#include "clients/JsonStateProducer.h"
#include <json.hpp>
#include <mutex>
#include <unordered_map>
#include <pigpio.h>

namespace mandeye
{

enum class LED {
	LED_GPIO_STOP_SCAN = 26,
	LED_GPIO_COPY_DATA = 19,
	LED_GPIO_CONTINOUS_SCANNING = 13
};

enum class BUTTON {
	BUTTON_STOP_SCAN = 5,
	BUTTON_CONTINOUS_SCANNING = 6
};

class GpioClient : public JsonStateProducer
{
public:
	//! Constructor
	//! @param sim if true hardware is not called
	GpioClient(bool sim);
	~GpioClient();

	//! serialize component state to API
	nlohmann::json produceStatus() override;
	std::string getJsonName() override;

	//! set led to given state
	void setLed(LED led, bool state);

	//! addcalback
	void addButtonCallback(BUTTON btn,
						   const std::string& callbackName,
						   const std::function<void()>& callback);

private:
	using Callbacks = std::unordered_map<std::string, std::function<void()>>;

	//! use simulated GPIOs instead real one
	bool m_useSimulatedGPIO{true};

	std::unordered_map<LED, bool> m_ledState{
		{LED::LED_GPIO_STOP_SCAN, false},
		{LED::LED_GPIO_COPY_DATA, false},
		{LED::LED_GPIO_CONTINOUS_SCANNING, false},
	};

	std::unordered_map<BUTTON, bool> m_buttonState{
		{BUTTON::BUTTON_STOP_SCAN, false},
		{BUTTON::BUTTON_CONTINOUS_SCANNING, false},
	};

	std::unordered_map<BUTTON, uint32_t> m_buttonLastPressTime{
		{BUTTON::BUTTON_STOP_SCAN, 0},
		{BUTTON::BUTTON_CONTINOUS_SCANNING, 0},
	};

	std::unordered_map<BUTTON, Callbacks> m_buttonsCallbacks;

	static void btnCallback(int gpio, int level, uint32_t tick, void* userdata);
	void initLed(LED led);
	void initButton(BUTTON btn);

	//! useful translations
	const std::unordered_map<LED, std::string> LedToName{
		{LED::LED_GPIO_STOP_SCAN, "LED_GPIO_STOP_SCAN"},
		{LED::LED_GPIO_COPY_DATA, "LED_GPIO_COPY_DATA"},
		{LED::LED_GPIO_CONTINOUS_SCANNING, "LED_GPIO_CONTINOUS_SCANNING"},
	};

	const std::unordered_map<std::string, LED> NameToLed{
		{"LED_GPIO_STOP_SCAN", LED::LED_GPIO_STOP_SCAN},
		{"LED_GPIO_COPY_DATA", LED::LED_GPIO_COPY_DATA},
		{"LED_GPIO_CONTINOUS_SCANNING", LED::LED_GPIO_CONTINOUS_SCANNING},
	};

	const std::unordered_map<BUTTON, std::string> ButtonToName{
		{BUTTON::BUTTON_STOP_SCAN, "BUTTON_STOP_SCAN"},
		{BUTTON::BUTTON_CONTINOUS_SCANNING, "BUTTON_CONTINOUS_SCANNING"},
	};

	const std::unordered_map<std::string, BUTTON> NameToButton{
		{"BUTTON_STOP_SCAN", BUTTON::BUTTON_STOP_SCAN},
		{"BUTTON_CONTINOUS_SCANNING", BUTTON::BUTTON_CONTINOUS_SCANNING},
	};

	std::mutex m_lock;
};
} // namespace mandeye