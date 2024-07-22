#include "clients/concrete/CamerasClient.h"
#include "state_management.h"
#include <opencv2/opencv.hpp>
#include <execution>
#include <ranges>

#define CAMERA_WIDTH 1920
#define CAMERA_HEIGHT 1080
#define FPS 2
#define IMAGE_FORMAT ".jpg"

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
	fullBufferLock.lock(); // writeBuffer is empty
	imagesWriterThread = std::thread(&CamerasClient::writeImages, this);
	imagesGrabberThread = std::thread(&CamerasClient::readImgesFromCaps, this);
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
	// tmp.set(CAP_PROP_FPS, 10); // test
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
	auto start = std::chrono::high_resolution_clock::now();
	for(auto& cap: caps)
		cap.grab();
	uint64_t timestamp = GetTimeStamp();

	std::vector<StampedImage> out;
	for(auto& cap: caps) {
		Mat tmp;
		cap.retrieve(tmp);
		out.push_back({tmp, timestamp});
	}
	std::cout << "Reading images took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() << " ms" << std::endl;
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
		// producer thread
		emptyBufferLock.lock();
		writeBuffer.clear();
		for(auto& img: currentImages)
			writeBuffer.push_back(img);
		fullBufferLock.unlock();
		auto end = std::chrono::high_resolution_clock::now();
		std::this_thread::sleep_for(delay - (end - begin));
	}
	for(auto& cap: caps)
		cap.release();
	fullBufferLock.unlock(); // let the consumer thread finish
}

//! This is the consumer thread
void CamerasClient::writeImages() {
	while(isRunning.load()) {
		fullBufferLock.lock();
		auto now = std::chrono::high_resolution_clock::now();
		writeImagesToDiskAndAddToBuffer(writeBuffer);
		std::cout << "Writing images to disk took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now).count() << " ms" << std::endl;
		writeBuffer.clear();
		emptyBufferLock.unlock();
	}
}

void CamerasClient::writeImagesToDiskAndAddToBuffer(const std::vector<StampedImage>& images)
{
	std::lock_guard<std::mutex> lock(bufferMutex);
	if(!isLogging.load())
		return; // recheck (strange things can happen in multithreading)

	for(int i=0; i< images.size(); i++) {
		ImageInfo tmp;
		tmp.path = generateTmpFilePath();
		tmp.cameraIndex = i;
		tmp.timestamp = images[i].timestamp;
		imwrite(tmp.path, images[i].image, {IMWRITE_JPEG_QUALITY, 100});
		savedImagesBuffer.push_back(tmp);
	}
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
void CamerasClient::readImgesFromCaps() {
	while(isRunning.load()) {
		std::vector<StampedImage> currentImages = readSyncedImages();
		imagesMutex.lock();
		imagesBuffer = currentImages;
		imagesMutex.unlock();
	}
}

} // namespace mandeye