#pragma once

#include "json.hpp"
#include <deque>
#include <mutex>

#include "clients/TimeStampReceiver.h"
#include "clients/LoggerClient.h"
#include "clients/SaveChunkInFileClient.h"
#include "minmea.h"
#include "thread"
#include <SerialPort.h>
#include <SerialStream.h>
namespace mandeye
{


class GNSSClient : public mandeye_utils::TimeStampReceiver, public mandeye_utils::SaveChunkInFileClient, public mandeye_utils::LoggerClient
{
private:
	void printBufferToFileString(std::stringstream& fss) override;
	std::string getFileExtension() override;
	std::string getFileIdentifier() override;

public:
	nlohmann::json produceStatus();

	//! Spins up a thread that reads from the serial port
	bool startListener(const std::string& portName, int baudRate);
	bool startListener();
	//! Start logging into the buffers
	void startLog() override;

	//! Stop logging into the buffers
	void stopLog() override;

	//! Retrieve all data from the buffer, in form of CSV lines
	std::deque<std::string> retrieveData();

private:
	std::mutex m_bufferMutex;
	std::deque<std::string> m_buffer;
	std::string m_lastLine;
	bool m_isLogging{false};
	minmea_sentence_gga lastGGA;
	LibSerial::SerialPort m_serialPort;
	LibSerial::SerialStream m_serialPortStream;
	std::thread m_serialPortThread;
	std::string m_portName;
	int m_baudRate {0};
	void worker();

	bool init_succes{false};

	//! Convert a minmea_sentence_gga to a CSV line
	std::string GgaToCsvLine(const minmea_sentence_gga& gga, double laserTimestamp);

};
} // namespace mandeye