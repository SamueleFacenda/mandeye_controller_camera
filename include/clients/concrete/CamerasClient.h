#ifndef MANDEYE_MULTISENSOR_CAMERASCLIENT_H
#define MANDEYE_MULTISENSOR_CAMERASCLIENT_H

#include "clients/LoggerClient.h"
#include "clients/SaveChunkToDirClient.h"
#include "clients/TimeStampReceiver.h"
#include <atomic>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace mandeye {

struct stampedImage {
	double timestamp;
	cv::Mat image;
};

class CamerasClient : public TimeStampReceiver, public SaveChunkToDirClient, public LoggerClient {
	public:
		CamerasClient(const std::vector<int>& cameraIndexes, const std::string& savingMediaPath);
		void receiveImages();
		void saveDumpedChunkToDirectory(const std::filesystem::path& directory, int chunk) override;
		void dumpChunkInternally() override;
		void startLog() override;
		void stopLog() override;

	private:
		std::filesystem::path tmpDir; // on the final media device
		std::vector<cv::VideoCapture> caps;
		std::mutex buffersMutex;
		std::vector<cv::VideoWriter> buffers;
		std::atomic<bool> isLogging{false};
		std::vector<std::filesystem::path> tmpFiles;
		std::vector<cv::VideoWriter> dumpBuffers; // needed by SaveChunkToDirClient
		std::vector<std::filesystem::path> dumpNames;

		void initializeVideoCapture(int index);
		void addImagesToBuffer(const std::vector<cv::Mat>& images, double timestamp);
		void initializeVideoWriter(int index);
		std::vector<cv::Mat> readSyncedImages();
		std::filesystem::path getTmpFilePath(int cameraIndex);
		static std::filesystem::path getFinalFilePath(const std::filesystem::path& outDir, int cameraIndex, int chunk);
};

} // namespace mandeye
#endif //MANDEYE_MULTISENSOR_CAMERASCLIENT_H
