#ifndef MANDEYE_MULTISENSOR_CAMERASCLIENT_H
#define MANDEYE_MULTISENSOR_CAMERASCLIENT_H

#include "clients/TimeStampReceiver.h"
#include "clients/SaveChunkToDirClient.h"
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace mandeye {

struct stampedImage {
	double timestamp;
	cv::Mat image;
};

class CamerasClient : public mandeye_utils::TimeStampReceiver, public mandeye_utils::SaveChunkToDirClient {
	public:
		explicit CamerasClient(const std::vector<int>& cameraIndexes);
		void receiveImages();
		void saveChunkToDirectory(const std::filesystem::path& directory, int chunk) override;

	private:
		std::vector<cv::VideoCapture> caps;
		std::mutex buffersMutex;
		std::vector<std::deque<stampedImage>> buffers;

		void initializeVideoCapture(int index);
		void saveBufferToDirectory(const std::filesystem::path& dirName, std::deque<stampedImage>& buffer, int cameraId);
		void addImagesToBuffer(const std::vector<cv::Mat>& images, double timestamp);
		std::vector<cv::Mat> readSynchedImages();

};

} // namespace mandeye
#endif //MANDEYE_MULTISENSOR_CAMERASCLIENT_H
