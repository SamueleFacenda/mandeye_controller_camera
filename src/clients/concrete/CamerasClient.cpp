#include "clients/concrete/CamerasClient.h"
#include "state_management.h"
#include <opencv2/opencv.hpp>
#include <execution>
#include <ranges>

#define MAX_CAMERA_INDEX 10
#define CAMERA_WIDTH 1920
#define CAMERA_HEIGHT 1200
#define FPS 5
#define IMAGE_FORMAT ".jpg"
// 5, 10, 15, 20, 25, 30, 60, 90
#define IMAGE_CAPTURE_FPS 15
#define OPENCV_IMAGE_BUFFER_SIZE 4
#define MAX_IMAGES_BUFFER_SIZE 16
#define SAFE_IMAGES_BUFFER_SIZE 8

namespace mandeye {

using namespace cv;

CamerasClient::CamerasClient(const std::string& savingMediaPath, ThreadMap& threadsList)
{
	isLogging.store(false);
	tmpDir = std::filesystem::path(savingMediaPath) / ".mandeye_cameras_tmp";
	if (!std::filesystem::is_directory(tmpDir) && !std::filesystem::create_directories(tmpDir))
		std::cerr << "Error creating directory '" << tmpDir << "'" << std::endl;

	for(int i  = 0; i <= MAX_CAMERA_INDEX; i++)
		initializeVideoCapture(i);
	std::cout << caps.size() << " cameras initialized" << std::endl;

	threadsList["Images Writer"] = std::make_shared<std::thread>(&CamerasClient::writeImages, this);
	threadsList["Images Grabber"] = std::make_shared<std::thread>(&CamerasClient::readImagesFromCaps, this);
}

void CamerasClient::initializeVideoCapture(int index) {
	VideoCapture tmp(index, CAP_V4L2); // on raspberry defaults to gstreamer, buggy
	if (!tmp.isOpened()) {
		std::cerr << "Error opening cap number " << index << std::endl;
		return;
	}
	// fourcc defaults to YUYV, it's too slow
	tmp.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
	tmp.set(CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
	tmp.set(CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);
	tmp.set(CAP_PROP_BUFFERSIZE, OPENCV_IMAGE_BUFFER_SIZE);
	tmp.set(CAP_PROP_FPS, IMAGE_CAPTURE_FPS);
	assert(tmp.get(CAP_PROP_FRAME_WIDTH) == CAMERA_WIDTH); // check if the camera accepted the resolution
	assert(tmp.get(CAP_PROP_FRAME_HEIGHT) == CAMERA_HEIGHT);
	caps.push_back(tmp);
	std::cout << "Initialized camera number " << index << std::endl;
}

void CamerasClient::saveDumpedChunkToDirectory(const std::filesystem::path& dirName, int chunkNumber)
{
	// photos_0001
	std::string chunkDir = "photos_" + std::string(chunkNumber ? 3 - (int) log10(chunkNumber) : 3, '0') + std::to_string(chunkNumber);
	std::filesystem::path outDir = dirName / chunkDir;
	if (!std::filesystem::is_directory(outDir) && !std::filesystem::create_directories(outDir)) {
		std::cerr << "Error creating directory '" << outDir << "'" << std::endl;
		return;
	}
	for(auto& img: dumpBuffer) {
		std::filesystem::path finalPath = getFinalFilePath(outDir, img.cameraIndex, chunkNumber, img.timestamp);
		std::filesystem::rename(img.path, finalPath);
	}
	dumpBuffer.clear();
}

void CamerasClient::dumpChunkInternally() {
	std::lock_guard<std::mutex> lock(bufferMutex);
	dumpBuffer = savedImagesBuffer;
	savedImagesBuffer.clear();
}

std::vector<StampedImage> CamerasClient::readSyncedImages()
{
	if (caps.size() == 0)
		// sleep to avoid busy loop
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto start = std::chrono::high_resolution_clock::now();
	for(auto& cap: caps)
		cap.grab();
	uint64_t timestamp = GetTimeStamp();

	std::vector<StampedImage> out;
	for(int i = 0; i < caps.size(); i++) {
		Mat tmp;
		caps[i].retrieve(tmp);
		out.push_back({tmp, timestamp, i});
	}
	// std::cout << "Reading images took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() << " ms" << std::endl;
	return out;
}

void CamerasClient::receiveImages() {
	std::vector<StampedImage> currentImages;
	auto delay = std::chrono::nanoseconds((uint64_t) (1e9 / FPS));

	while(isRunning.load()) {
		auto begin = std::chrono::high_resolution_clock::now();
		if (!isLogging.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue; // do not waste CPU time if we are not logging
		}

		imagesMutex.lock();
		currentImages = imagesBuffer;
		imagesMutex.unlock();
		for(auto& img: currentImages)
			writeBuffer.push(img);
		auto end = std::chrono::high_resolution_clock::now();
		// std::cout << "Sleep for " << std::chrono::duration_cast<std::chrono::milliseconds>(delay - (end - begin)).count() << std::endl;
		if (delay < end - begin)
			std::cout << "Warning!! Negative sleep time, we are late (probably too slow writing speed)" << std::endl;
		std::this_thread::sleep_for(delay - (end - begin));
	}
	writeBuffer.stop();
}

//! This is the consumer thread
void CamerasClient::writeImages() {
	StampedImage tmp;
	ImageInfo tmpInfo;
	while(isRunning.load()) {
		if (writeBuffer.size() > MAX_IMAGES_BUFFER_SIZE) {
			writeBuffer.keepN(SAFE_IMAGES_BUFFER_SIZE);
			std::cout << "Dropping images, buffer is full" << std::endl;
		}

		tmp = writeBuffer.pop();
		if (tmp.cameraIndex < 0)
			continue;

		auto now = std::chrono::high_resolution_clock::now();
		tmpInfo = preWriteImageToDisk(tmp);
		{
			std::lock_guard<std::mutex> lock(bufferMutex);
			savedImagesBuffer.push_back(tmpInfo);
		}
		std::cout << "Writing image to disk took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now).count() << " ms" << std::endl;
	}
}

ImageInfo CamerasClient::preWriteImageToDisk(const StampedImage& img)
{
	std::lock_guard<std::mutex> lock(bufferMutex);

	ImageInfo tmp{
		.path = generateTmpFilePath(),
		.timestamp = img.timestamp,
		.cameraIndex = img.cameraIndex
	};
	imwrite(tmp.path, img.image, {IMWRITE_JPEG_QUALITY, 100});
	return tmp;
}

void CamerasClient::startLog() {
	std::lock_guard<std::mutex> lock(bufferMutex);
	savedImagesBuffer.clear();
	isLogging.store(true);
}

void CamerasClient::stopLog() {
	isLogging.store(false);
	std::lock_guard<std::mutex> lock(bufferMutex);
	savedImagesBuffer.clear();
}

std::filesystem::path CamerasClient::generateTmpFilePath()
{
	return tmpDir / ("tmpImage_" +std::to_string(tmpImageCounter++) + IMAGE_FORMAT);
}

std::filesystem::path CamerasClient::getFinalFilePath(const std::filesystem::path& outDir, int cameraIndex, int chunk, uint64_t timestamp)
{
	// camera_0_chunk_0001_ts_1234567890.jpg
	return outDir / ("camera_" + std::to_string(cameraIndex) +
					 // "_chunk_" + std::string(chunk ? 3 - (int) log10(chunk) : 3, '0') + std::to_string(chunk) +
					 "_ts_" + std::to_string(timestamp) + IMAGE_FORMAT);
}

void CamerasClient::readImagesFromCaps() {
	while(isRunning.load()) {
		auto start = std::chrono::high_resolution_clock::now();

		std::vector<StampedImage> currentImages = readSyncedImages();
		imagesMutex.lock();
		imagesBuffer = currentImages;
		imagesMutex.unlock();

		auto end = std::chrono::high_resolution_clock::now();
		auto fps = 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		int millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		if (millis > 100)
			std::cout << "Warning!! Reading images took " << millis << " ms" << std::endl;
	}
	for(auto& cap: caps)
		cap.release();
}

} // namespace mandeye
