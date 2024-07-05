#include "clients/concrete/CamerasClient.h"
#include "state_management.h"
#include <opencv2/opencv.hpp>
#include <execution>
#include <ranges>

#define CAMERA_WIDTH 1920
#define CAMERA_HEIGHT 1080
#define EXPECTED_FPS 10

namespace mandeye {

using namespace cv;

CamerasClient::CamerasClient(const std::vector<int>& cameraIndexes, const std::string& savingMediaPath)
{
	isLogging.store(false);
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
		tmpFiles.emplace_back("");
	}
}

void CamerasClient::initializeVideoCapture(int index) {
	VideoCapture tmp(index, CAP_V4L2); // on raspberry defaults to gstreamer, buggy
	if (!tmp.isOpened()) {
		std::cerr << "Error opening cap number " << index << std::endl;
		return;
	}
	// fourcc defaults to YUYV, which is buggy in the resolution selection
	tmp.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
	tmp.set(CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
	tmp.set(CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);
	assert(tmp.get(CAP_PROP_FRAME_WIDTH) == CAMERA_WIDTH); // check if the camera accepted the resolution
	assert(tmp.get(CAP_PROP_FRAME_HEIGHT) == CAMERA_HEIGHT);
	caps.push_back(tmp);
	std::cout << "Initialized camera number " << index << std::endl;
}

void CamerasClient::saveDumpedChunkToDirectory(const std::filesystem::path& dirName, int chunkNumber)
{
	// parallel for
	auto range = std::views::iota(0, (int) dumpBuffers.size());
	std::for_each(std::execution::par_unseq, range.begin(), range.end(), [&](int i) {
		// auto start = std::chrono::steady_clock::now();
		dumpBuffers[i].release(); // this takes ~0.6s with FFV1, pretty good
		// std::cout << "Time to save chunk " << i << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "ms\n";
		std::filesystem::rename(dumpNames[i], getFinalFilePath(dirName, i, chunkNumber));
	});
}

void CamerasClient::dumpChunkInternally() {
	dumpBuffers.clear();
	dumpNames.clear();
	std::lock_guard<std::mutex> lock(buffersMutex);
	for(int i=0; i<buffers.size(); i++) {
		VideoWriter empty;
		dumpBuffers.push_back(buffers[i]);
		dumpNames.push_back(tmpFiles[i]);
		buffers[i] = empty;
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
	// add random number to avoid conflicts
	return tmpDir / ("camera_" + std::to_string(cameraIndex) + "_" +std::to_string((int)(rand()%10000)) + ".mkv");
}

std::filesystem::path CamerasClient::getFinalFilePath(const std::filesystem::path& outDir, int cameraIndex, int chunk) {
	// camera_0_chunk_0001.mp4
	return outDir / ("camera_" + std::to_string(cameraIndex) + "_chunk_" + std::string(chunk ? 3 - (int) log10(chunk) : 3, '0') + std::to_string(chunk) + ".mkv");
}

void CamerasClient::initializeVideoWriter(int index) {
	// h264 can be changed to something properly lossless or h265 (better encoding, x10/x20 encoding time, according to some random blog post I found)
	// avc1 or mp4v can be used for h264
	// int fourcc = VideoWriter::fourcc('a','v','c','1');
	int fourcc = VideoWriter::fourcc('F','F','V','1');
	tmpFiles[index] = getTmpFilePath(index);
	buffers[index].open(tmpFiles[index], fourcc, EXPECTED_FPS, Size(CAMERA_WIDTH, CAMERA_HEIGHT));
}

} // namespace mandeye