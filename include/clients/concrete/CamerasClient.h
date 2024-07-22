#ifndef MANDEYE_MULTISENSOR_CAMERASCLIENT_H
#define MANDEYE_MULTISENSOR_CAMERASCLIENT_H

#include "clients/IterableToFileSaver.h"
#include "clients/LoggerClient.h"
#include "clients/SaveChunkToDirClient.h"
#include "clients/TimeStampReceiver.h"
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
		std::mutex bufferMutex;
		std::mutex imagesMutex;
		std::vector<StampedImage> imagesBuffer;
		std::vector<ImageInfo> savedImagesBuffer;
		std::atomic<bool> isLogging{false};
		int tmpImageCounter = 0;
		std::vector<ImageInfo> dumpBuffer; // needed by SaveChunkToDirClient
		std::thread imagesWriterThread, imagesGrabberThread;
		std::vector<StampedImage> writeBuffer;
		std::mutex emptyBufferLock, fullBufferLock; // there is the imagesWriterThread and the main thread, consuming and producing respectively

		void initializeVideoCapture(int index);
		void writeImagesToDiskAndAddToBuffer(const std::vector<StampedImage>& images);
		void writeImages(); // thread
		void readImgesFromCaps(); // Images grabber thread
		std::vector<StampedImage> readSyncedImages();
		std::filesystem::path generateTmpFilePath();
		static std::filesystem::path getFinalFilePath(const std::filesystem::path& outDir, int cameraIndex, int chunk, uint64_t timestamp);
};

} // namespace mandeye
#endif //MANDEYE_MULTISENSOR_CAMERASCLIENT_H
