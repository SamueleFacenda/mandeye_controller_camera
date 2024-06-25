#include "clients/concrete/CamerasClient.h"
#include "state_management.h"
#include <opencv2/opencv.hpp>

namespace mandeye {

using namespace cv;

CamerasClient::CamerasClient(const std::vector<int>& cameraIndexes, const std::string& savingMediaPath)
{
	tmpDir = std::filesystem::path(savingMediaPath) / ".mandeye_cameras_tmp";
	if (!std::filesystem::is_directory(tmpDir) && !std::filesystem::create_directories(tmpDir)) {
		std::cerr << "Error creating directory '" << tmpDir << "'" << std::endl;
	}
	for(int index: cameraIndexes)
		initializeVideoCapture(index);

	std::lock_guard<std::mutex> lock(buffersMutex);
	for(int i=0; i<caps.size(); i++) {
		VideoWriter tmp;
		buffers.push_back(tmp);
	}
}

void CamerasClient::initializeVideoCapture(int index) {
	VideoCapture tmp(index);
	if (!tmp.isOpened()) {
		std::cerr << "Error opening cap number " << index << std::endl;
		return;
	}
	caps.push_back(tmp);
	std::cout << "Initialized camera number " << index << std::endl;
}

void CamerasClient::saveChunkToDirectory(const std::filesystem::path& dirName, int chunkNumber)
{

	std::lock_guard<std::mutex> lock(buffersMutex);
	for(int i=0; i<buffers.size(); i++) {
		buffers[i].release();
		std::filesystem::rename(getTmpFilePath(i), getFinalFilePath(dirName, i, chunkNumber));
		initializeVideoWriter(i);
	}
}

std::vector<Mat> CamerasClient::readSyncedImages()
{
	for(auto& cap: caps)
		cap.grab();

	std::vector<Mat> out;
	Mat tmp;
	for(auto& cap: caps) {
		cap.retrieve(tmp);
		out.push_back(tmp);
	}
	return out;
}

void CamerasClient::receiveImages() {
	double lastTimestamp = GetTimeStamp(), currentTimestamp;
	std::vector<Mat> currentImages;
	while(isRunning.load()) {
		if (!isLogging.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue; // do not waste CPU time if we are not logging
		}

		// check for the timestamp update (when a pointcloud is received)
		// should happen 10 times per second (I think that the cameras are faster)
		// When the timestamp is updated I add the latest images to the buffer
		currentImages = readSyncedImages();
		currentTimestamp = GetTimeStamp();

		if (currentTimestamp != lastTimestamp) {
			// std::cout << "Timestamp updated: delay = " << (currentTimestamp - lastTimestamp) << " seconds\n";
			lastTimestamp = currentTimestamp;
			addImagesToBuffer(currentImages, lastTimestamp);
		}
	}
	for(auto& cap: caps)
		cap.release();
}

void CamerasClient::addImagesToBuffer(const std::vector<Mat>& images, double timestamp) {
	std::lock_guard<std::mutex> lock(buffersMutex);
	if(!isLogging.load())
		return; // recheck (strange things can happen in multithreading)

	for(int i=0; i<buffers.size(); i++)
		buffers[i] << images[i];
	// std::cout << "Added images to buffer, current size: " << buffers[0].size() << std::endl;
}

void CamerasClient::startLog() {
	std::lock_guard<std::mutex> lock(buffersMutex);
	for(int index=0; index<buffers.size(); index++)
		initializeVideoWriter(index);
	isLogging.store(true);
}

void CamerasClient::stopLog() {
	isLogging.store(false);
	std::lock_guard<std::mutex> lock(buffersMutex);
	for(auto& buffer: buffers)
		buffer.release();
}

std::filesystem::path CamerasClient::getTmpFilePath(int cameraIndex) {
	return tmpDir / ("camera_" + std::to_string(cameraIndex) + ".mp4");
}

std::filesystem::path CamerasClient::getFinalFilePath(const std::filesystem::path& outDir, int cameraIndex, int chunk) {
	// camera_0_chunk_0001.mp4
	return outDir / ("camera_" + std::to_string(cameraIndex) + "_chunk_" + std::string(chunk ? 3 - (int) log10(chunk) : 3, '0') + std::to_string(chunk) + ".mp4");
}

void CamerasClient::initializeVideoWriter(int index) {
	// h264 can be changed to something properly lossless or h265 (better encoding, x10/x20 encoding time, according to some random blog post I found)
	buffers[index].open(getTmpFilePath(index), VideoWriter::fourcc('X','2','6','4'), 10, Size(1920, 1080));
}

} // namespace mandeye