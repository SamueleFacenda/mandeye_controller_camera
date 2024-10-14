#ifndef MANDEYE_MULTISENSOR_CAMERASCLIENT_H
#define MANDEYE_MULTISENSOR_CAMERASCLIENT_H

#include "clients/IterableToFileSaver.h"
#include "clients/LoggerClient.h"
#include "clients/SaveChunkToDirClient.h"
#include "clients/TimeStampReceiver.h"
#include "utils/BlockingQueue.h"
#include "livox_types.h"
#include <atomic>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <thread>

namespace mandeye {

struct ImageInfo {
	std::filesystem::path path;
	uint64_t timestamp;
	int cameraIndex;
};

struct StampedImage {
	cv::Mat image;
	uint64_t timestamp;
	int cameraIndex = -1;
};

class CamerasClient : public TimeStampReceiver, public SaveChunkToDirClient, public LoggerClient {
	public:
		CamerasClient(const std::string& savingMediaPath, ThreadMap& threadsList); // threadsList for joining the threads at shutdown
		void receiveImages();
		void saveDumpedChunkToDirectory(const std::filesystem::path& directory, int chunk) override;
		void dumpChunkInternally() override;
		void startLog() override;
		void stopLog() override;

	private:
		std::filesystem::path tmpDir; // on the final media device
		std::vector<cv::VideoCapture> caps;
		std::mutex bufferMutex;
		std::mutex imagesMutex;
		std::vector<StampedImage> imagesBuffer;
		std::vector<ImageInfo> savedImagesBuffer;
		std::atomic<bool> isLogging{false};
		int tmpImageCounter = 0;
		std::vector<ImageInfo> dumpBuffer; // needed by SaveChunkToDirClient
		utils::BlockingQueue<StampedImage> writeBuffer;

		void initializeVideoCapture(int index);
		ImageInfo preWriteImageToDisk(const StampedImage& img);
		void writeImages(); // thread
		void readImagesFromCaps(); // Images grabber thread
		std::vector<StampedImage> readSyncedImages();
		std::filesystem::path generateTmpFilePath();
		static std::filesystem::path getFinalFilePath(const std::filesystem::path& outDir, int cameraIndex, uint64_t timestamp);
};

} // namespace mandeye
#endif //MANDEYE_MULTISENSOR_CAMERASCLIENT_H
