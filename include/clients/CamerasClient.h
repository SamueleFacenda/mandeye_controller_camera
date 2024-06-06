#ifndef MANDEYE_MULTISENSOR_CAMERASCLIENT_H
#define MANDEYE_MULTISENSOR_CAMERASCLIENT_H

#include "TimeStampReceiver.h"
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace mandeye {

struct stampedImage {
	double timestamp;
	cv::Mat image;
};

class CamerasClient : public mandeye_utils::TimeStampReceiver {
	public:
		explicit CamerasClient(const std::vector<int>& cameraIndexes);
		void saveBuffersToDirectory(const std::string& dirName, int chunkNumber);
	private:
		std::vector<cv::VideoCapture> caps;
		std::mutex buffersMutex;
		std::vector<std::deque<stampedImage>> buffers;

		void initializeVideoCapture(int index);
		void saveBufferToDirectory(const std::filesystem::path& dirName, std::deque<stampedImage>& buffer, int cameraId);
		void addImagesToBuffer(const std::vector<cv::Mat>& images, double timestamp);
		std::vector<cv::Mat> readSynchedImages();

		void receiveImages();
};

} // namespace mandeye
#endif //MANDEYE_MULTISENSOR_CAMERASCLIENT_H
