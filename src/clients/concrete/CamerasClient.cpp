#include "clients/concrete/CamerasClient.h"
#include "state_management.h"
#include <opencv2/opencv.hpp>

namespace mandeye {

using namespace cv;

CamerasClient::CamerasClient(const std::vector<int>& cameraIndexes) {
	for(int index: cameraIndexes)
		initializeVideoCapture(index);
}

void CamerasClient::initializeVideoCapture(int index) {
	VideoCapture tmp;
	caps.push_back(tmp);
	caps.back().open(index);
	if (!caps.back().isOpened()) {
		std::cerr << "Error opening cap number " << index << std::endl;
		caps.pop_back();
		return;
	}

	std::deque<stampedImage> buffer;
	buffers.push_back(buffer);
}

void CamerasClient::saveChunkToDirectory(const std::filesystem::path& dirName, int chunkNumber)
{
	using namespace std::filesystem;
	char chunkDir[64];
	snprintf(chunkDir, 64, "images%04d", chunkNumber);
	path outDir = dirName / path(chunkDir);
	if (!create_directory(outDir)) {
		std::cerr << "Error creating directory '" << outDir << "'" << std::endl;
		return; // do not clean up the buffers, keep images in memory
	}
	for(int i=0; i<buffers.size(); i++)
		saveBufferToDirectory(outDir, buffers[i], i);
}

void CamerasClient::saveBufferToDirectory(const std::filesystem::path& dirName, std::deque<stampedImage>& buffer, int cameraId) {
	stampedImage tmp;
	std::filesystem::path imagePath;
	char filename[64];
	while(!buffer.empty()) {
		tmp = buffer.front();
		snprintf(filename, 64, "camera_%d_stamp_%d.png", cameraId, (int) (tmp.timestamp * 1e9));
		imagePath = dirName / filename;
		imwrite(imagePath, tmp.image);
		buffer.pop_front();
	}
}


std::vector<Mat> CamerasClient::readSynchedImages()
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
	while(isRunning) {
		if (!isLogging.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue; // do not waste CPU time if we are not logging
		}

		// check for the timestamp update (when a pointcloud is received)
		// should happen 10 times per second (I think that the cameras are faster)
		// When the timestamp is updated I add the latest images to the buffer
		currentImages = readSynchedImages();
		currentTimestamp = GetTimeStamp();

		if (currentTimestamp != lastTimestamp) {
			lastTimestamp = currentTimestamp;
			addImagesToBuffer(currentImages, lastTimestamp);
		}
	}
}

void CamerasClient::addImagesToBuffer(const std::vector<Mat>& images, double timestamp) {
	std::lock_guard<std::mutex> lock(buffersMutex);
	stampedImage tmp;

	tmp.timestamp = timestamp;
	for(int i=0; i<buffers.size(); i++) {
		tmp.image = images[i];
		buffers[i].push_back(tmp);
	}
}

void CamerasClient::startLog() {
	isLogging.store(true);
}
void CamerasClient::stopLog() {
	isLogging.store(false);
}

} // namespace mandeye