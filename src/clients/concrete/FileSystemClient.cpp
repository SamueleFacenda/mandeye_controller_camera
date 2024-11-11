#include "clients/concrete/FileSystemClient.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <unistd.h>

using namespace std::filesystem;

namespace mandeye
{

FileSystemClient::FileSystemClient(std::string repository) : m_repository(std::move(repository)) {
	m_nextId = GetIdFromManifest();
}

nlohmann::json FileSystemClient::produceStatus() {
	nlohmann::json data;
	data["FileSystemClient"]["repository"] = m_repository;
	float free_mb = 0;

	try {
		free_mb = CheckAvailableSpace();
	}
	catch(std::filesystem::filesystem_error& e) {
		data["FileSystemClient"]["error"] = e.what();
	}
	data["FileSystemClient"]["free_megabytes"] = free_mb;
	data["FileSystemClient"]["free_str"] = ConvertMbToText(free_mb);

	try {
		data["FileSystemClient"]["m_nextId"] = m_nextId;
	}
	catch(std::filesystem::filesystem_error& e) {
		data["FileSystemClient"]["error"] = e.what();
	}
	try {
		data["FileSystemClient"]["writable"] = GetIsWritable();
	}
	catch(std::filesystem::filesystem_error& e) {
		data["FileSystemClient"]["error"] = e.what();
	}

	try {
		data["FileSystemClient"]["dirs"] = GetDirectories();
	}
	catch(std::filesystem::filesystem_error& e) {
		data["FileSystemClient"]["error"] = e.what();
	}
	return data;
}

//! Test is writable
float FileSystemClient::CheckAvailableSpace() {
	std::error_code ec;
	const space_info si = space(m_repository, ec);
	if(ec.value() == 0) {
		const float f = static_cast<float>(si.free) / (1024 * 1024);
		return std::round(f);
	}
	return -1.f;
}

std::string FileSystemClient::ConvertMbToText(float mb) {
	std::stringstream tmp;
	tmp << std::setprecision(1) << std::fixed << mb / 1024 << "GB";
	return tmp.str();
}

int FileSystemClient::GetIdFromManifest() {
	path versionPath = path(m_repository) / path(versionFilename);
	std::ofstream versionOfStream(versionPath);
	versionOfStream << "Version 0.4" << std::endl;
	versionOfStream.close();

	path manifest = path(m_repository) / path(manifestFilename);
	std::unique_lock<std::mutex> lck(m_mutex);

	std::ifstream manifestIfStream(manifest);
	if(manifestIfStream.good() && manifestIfStream.is_open()) {
		int id = 0;
		manifestIfStream >> id;
		return id;
	}

	// first time, create the file
	std::ofstream manifestOfStream(manifest);
	if(manifestOfStream.good() && manifestOfStream.is_open()) {
		int id = 0;
		manifestOfStream << id << std::endl;
		return id;
	}

	return -1;
}

int FileSystemClient::GetNextIdFromManifest() {
	path manifest = path(m_repository) / path(manifestFilename);
	int32_t id = GetIdFromManifest();
	id++;
	m_nextId = id;
	std::ofstream manifestOfStream;
	manifestOfStream.open(manifest.c_str());
	if(manifestOfStream.good() && manifestOfStream.is_open()) {
		manifestOfStream << id << std::endl;
		return id;
	}
	return id;
}

bool FileSystemClient::CreateDirectoryForContinousScanning(std::string& writable_dir, const int& id_manifest) {
	std::string ret;

	if(!GetIsWritable())
		return false;

	auto id = id_manifest;
	char dirName[256];
	snprintf(dirName, 256, "continousScanning_%04d", id);
	path newDirPath = path(m_repository) / path(dirName);
	std::cout << "Creating directory " << newDirPath.string() << std::endl;
	std::error_code ec;
	create_directories(newDirPath, ec);
	m_error = ec.message();
	if(ec.value() != 0)
		return false;
	if(newDirPath.string().empty())
		return false;

	writable_dir = newDirPath.string();
	return true;
}

bool FileSystemClient::CreateDirectoryForStopScans(std::string& writable_dir, int& id_manifest) {
	std::string ret;

	if(!GetIsWritable())
		return false;

	id_manifest = GetNextIdFromManifest() - 1;
	char dirName[256];
	snprintf(dirName, 256, "stopScans_%04d", id_manifest);
	path newDirPath = path(m_repository) / path(dirName);
	std::cout << "Creating directory " << newDirPath.string() << std::endl;
	std::error_code ec;
	create_directories(newDirPath, ec);
	m_error = ec.message();
	if(ec.value() != 0)
		return false;
	if(newDirPath.string().empty())
		return false;

	writable_dir = newDirPath.string();
	return true;
}

std::vector<std::string> FileSystemClient::GetDirectories() {
	std::unique_lock<std::mutex> lck(m_mutex);
	std::vector<std::string> fn;

	for(const auto& entry : recursive_directory_iterator(m_repository)) {
		if(entry.is_regular_file()) {
			double size = file_size(entry);
			size /= (1024 * 1204);
			fn.push_back(entry.path().string() + " " + std::to_string(size) + " Mb");
		} else
			fn.push_back(entry.path().string());
	}
	std::sort(fn.begin(), fn.end());
	return fn;
}

bool FileSystemClient::GetIsWritable() {
	return access(m_repository.c_str(), W_OK) == 0;
}

std::string FileSystemClient::getJsonName() {
	return "fs";
}

} // namespace mandeye