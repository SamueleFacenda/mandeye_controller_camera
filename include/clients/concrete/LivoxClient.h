#ifndef MANDEYE_MULTISENSOR_LIVOXCLIENT_H
#define MANDEYE_MULTISENSOR_LIVOXCLIENT_H

#include "clients/IterableToFileSaver.h"
#include "clients/JsonStateProducer.h"
#include "clients/LoggerClient.h"
#include "clients/SaveChunkToDirClient.h"
#include "clients/TimeStampProvider.h"
#include "livox_types.h"
#include <json.hpp>
#include <livox_lidar_def.h>
#include <mutex>
#include <thread>
#include <tiff.h>

namespace mandeye
{

class LivoxClient : public SaveChunkToDirClient, public TimeStampProvider, public LoggerClient, public JsonStateProducer
{
public:
	LivoxClient();

	nlohmann::json produceStatus() override;
	std::string getJsonName() override;

	//! starts LivoxSDK2, interface is IP of listen interface (IP of network cards with Livox connected
	bool startListener(const std::string& interfaceIp);

	//! Start log to memory data from Lidar and IMU
	void startLog() override;

	//! Stops log to memory data from Lidar and IMU
	void stopLog() override;

	std::pair<LivoxPointsBufferPtr, LivoxIMUBufferPtr> retrieveData();

	//! Return current mapping from serial number to lidar id
	std::unordered_map<uint32_t, std::string> getSerialNumberToLidarIdMapping() const;

	// TimeStampProvider overrides ...
	uint64 getTimestamp() override;

	// periodically ask lidars for status
	void testThread();

	void saveDumpedChunkToDirectory(const std::filesystem::path& directory, int chunk) override;
	void dumpChunkInternally() override;

private:
	bool isDone{false};
	std::thread m_livoxWatchThread;
	std::mutex m_bufferImuMutex;
	std::mutex m_bufferLidarMutex;

	LivoxPointsBufferPtr m_bufferLivoxPtr{nullptr};
	LivoxIMUBufferPtr m_bufferIMUPtr{nullptr};

	LivoxPointsBufferPtr dumpedBufferLivoxPtr{nullptr}; // needed by SaveChunkToDirClient

	std::mutex m_timestampMutex;
	uint64_t m_timestamp{};

	//! Multilovx support
	mutable std::mutex m_lidarInfoMutex;
	std::unordered_map<uint32_t, uint64_t> m_recivedImuMsgs;
	std::unordered_map<uint32_t, uint64_t> m_recivedPointMessages;
	std::unordered_map<uint32_t, LivoxLidarInfo> m_LivoxLidarInfo;
	std::unordered_map<uint32_t, int32_t> m_LivoxLidarWorkMode;
	std::unordered_map<uint32_t, int32_t> m_LivoxLidarTimeSync;


	std::unordered_map<uint32_t, uint64_t> m_handleToLastTimestamp;
	std::unordered_map<uint32_t, std::string> m_handleToSerialNumber;


	//! This is a set of serial numbers that we have already seen, its used to find lidarId
	std::set<std::string> m_serialNumbers;

	bool init_succes{false};
	uint64_t systemTimestampDelay{};

	//! converts a handle to a lidar id. The logic is as follows:
	//! id is zero for lidar with smallest Serial number
	//! @param handle the handle to convert
	uint16_t handleToLidarId(uint32_t handle) const;

	IterableToFileSaver<std::deque, LivoxIMU> imuIteratorToFileSaver;
	IterableToFileSaver<std::unordered_map, uint32_t, std::string> lidarIteratorToFileSaver;
	std::shared_ptr<std::deque<LivoxIMU>> dumpedBufferImuPtr;
	std::unordered_map<uint32_t, std::string> dumpedSerialNumberToLidarIdMapping;


	static constexpr char config[] =
R"(
{
	"MID360": {
		"lidar_net_info" : {
			"cmd_data_port": 56100,
			"push_msg_port": 56200,
			"point_data_port": 56300,
			"imu_data_port": 56400,
			"log_data_port": 56500
		},
		"host_net_info" : {
			"cmd_data_ip" : "${HOSTIP}",
			"cmd_data_port": 56101,
			"push_msg_ip": "${HOSTIP}",
			"push_msg_port": 56201,
			"point_data_ip": "${HOSTIP}",
			"point_data_port": 56301,
			"imu_data_ip" : "${HOSTIP}",
			"imu_data_port": 56401,
			"log_data_ip" : "${HOSTIP}",
			"log_data_port": 56501
		}
	}
}

)";

	// callbacks
	static void PointCloudCallback(uint32_t handle,
								   const uint8_t dev_type,
								   LivoxLidarEthernetPacket* data,
								   void* client_data);

	static void ImuDataCallback(uint32_t handle,
								const uint8_t dev_type,
								LivoxLidarEthernetPacket* data,
								void* client_data);

	static void WorkModeCallback(livox_status status,
								 uint32_t handle,
								 LivoxLidarAsyncControlResponse* response,
								 void* client_data);

	static void SetIpInfoCallback(livox_status status,
								  uint32_t handle,
								  LivoxLidarAsyncControlResponse* response,
								  void* client_data);

	static void RebootCallback(livox_status status,
							   uint32_t handle,
							   LivoxLidarRebootResponse* response,
							   void* client_data);

	static void QueryInternalInfoCallback(livox_status status,
										  uint32_t handle,
										  LivoxLidarDiagInternalInfoResponse* packet,
										  void* client_data);

	static void
	LidarInfoChangeCallback(const uint32_t handle, const LivoxLidarInfo* info, void* client_data);
};
} // namespace mandeye

#endif //MANDEYE_MULTISENSOR_LIVOXCLIENT_H