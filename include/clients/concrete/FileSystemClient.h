#pragma once

#include "clients/JsonStateProducer.h"
#include <json.hpp>
#include "livox_lidar_def.h"
#include <deque>
#include <mutex>
#include <string>

namespace mandeye
{

class FileSystemClient : public JsonStateProducer {
	constexpr static char manifestFilename[]{"mandala_manifest.txt"};
	constexpr static char versionFilename[]{"version.txt"};

public:
	FileSystemClient(std::string repository);
	nlohmann::json produceStatus() override;
	std::string getJsonName() override;

	//! Test is writable
	float CheckAvailableSpace();

	//! Create Counter file
	int GetIdFromManifest();

	//! Create Counter file
	int GetNextIdFromManifest();

	//! Get is writable
	bool GetIsWritable();

	std::vector<std::string> GetDirectories();

	bool CreateDirectoryForContinousScanning(std::string&, const int&);

	bool CreateDirectoryForStopScans(std::string&, int& id_manifest);

private:
	int32_t m_nextId{0};
	static std::string ConvertMbToText(float mb);
	std::string m_repository;
	std::string m_error;
	std::mutex m_mutex;
};
} // namespace mandeye