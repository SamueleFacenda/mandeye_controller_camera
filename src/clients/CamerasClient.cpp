#include "clients/CamerasClient.h"
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

void CamerasClient::saveBuffersToDirectory(const std::string& dirName, int chunkNumber) {

}

void CamerasClient::saveBufferToDirectory(const std::string& dirName) { }


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

} // namespace mandeye