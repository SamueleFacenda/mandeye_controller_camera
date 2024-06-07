#pragma once

#include "clients/TimeStampProvider.h"
#include "clients/SaveChunkToDirClient.h"
#include "clients/LoggerClient.h"
#include "clients/concrete/ImuFileSaver.h"
#include "clients/concrete/LidarFileSaver.h"
#include "json.hpp"
#include "livox_lidar_def.h"
#include <deque>
#include <mutex>
#include <set>
#include <thread>
namespace mandeye
{
struct LivoxPoint
{
	LivoxLidarCartesianHighRawPoint point;
	uint64_t timestamp;
	uint8_t line_id;
	uint16_t laser_id;
};

struct LivoxIMU
{
	LivoxLidarImuRawPoint point;
	uint64_t timestamp;
	uint16_t laser_id;
};

const std::unordered_map<int32_t, char*> WorkModeToStr{
	{LivoxLidarWorkMode::kLivoxLidarNormal, "kLivoxLidarNormal"},
	{LivoxLidarWorkMode::kLivoxLidarWakeUp, "kLivoxLidarWakeUp"},
	{LivoxLidarWorkMode::kLivoxLidarSleep, "kLivoxLidarSleep"},
	{LivoxLidarWorkMode::kLivoxLidarError, "kLivoxLidarError"},
	{LivoxLidarWorkMode::kLivoxLidarPowerOnSelfTest, "kLivoxLidarPowerOnSelfTest"},
	{LivoxLidarWorkMode::kLivoxLidarMotorStarting, "kLivoxLidarMotorStarting"},
	{LivoxLidarWorkMode::kLivoxLidarMotorStoping, "kLivoxLidarMotorStoping"},
	{LivoxLidarWorkMode::kLivoxLidarUpgrade, "kLivoxLidarUpgrade"},
	{-1, "FailedToGetWorkMode"},

};

using LivoxPointsBuffer = std::deque<LivoxPoint>;
using LivoxPointsBufferPtr = std::shared_ptr<std::deque<LivoxPoint>>;
using LivoxPointsBufferConstPtr = std::shared_ptr<const std::deque<LivoxPoint>>;
using LivoxIMUBuffer = std::deque<LivoxIMU>;
using LivoxIMUBufferPtr = std::shared_ptr<std::deque<LivoxIMU>>;
using LivoxIMUBufferConstPtr = std::shared_ptr<const std::deque<LivoxIMU>>;

class LivoxClient : public mandeye_utils::SaveChunkToDirClient, public mandeye_utils::TimeStampProvider, public mandeye_utils::LoggerClient
{
public:
	nlohmann::json produceStatus();

	//! starts LivoxSDK2, interface is IP of listen interface (IP of network cards with Livox connected
	bool startListener(const std::string& interfaceIp);

	//! Start log to memory data from Lidar and IMU
	void startLog() override;

	//! Stops log to memory data from Lidar and IMU
	void stopLog() override;

	std::pair<LivoxPointsBufferPtr, LivoxIMUBufferPtr> retrieveData();

	//! Return current mapping from serial number to lidar id
	std::unordered_map<uint32_t, std::string> getSerialNumberToLidarIdMapping() const;

	// mandeye_utils::TimeStampProvider overrides ...
	double getTimestamp() override;

	// periodically ask lidars for status
	void testThread();

	void saveChunkToDirectory(const std::filesystem::path& directory, int chunk) override;

private:
	bool isDone{false};
	std::thread m_livoxWatchThread;
	std::mutex m_bufferImuMutex;
	std::mutex m_bufferLidarMutex;

	LivoxPointsBufferPtr m_bufferLivoxPtr{nullptr};
	LivoxIMUBufferPtr m_bufferIMUPtr{nullptr};

	std::mutex m_timestampMutex;
	uint64_t m_timestamp;

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

	//! converts a handle to a lidar id. The logic is as follows:
	//! id is zero for lidar with smallest Serial number
	//! @param handle the handle to convert
	uint16_t handleToLidarId(uint32_t handle) const;

	mandeye_utils::ImuFileSaver imuFileSaver;
	mandeye_utils::LidarFileSaver lidarFileSaver;


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
	},
	"HAP": {
		"lidar_net_info" : {
			"cmd_data_port": 56000,
			"push_msg_port": 0,
			"point_data_port": 57000,
			"imu_data_port": 58000,
			"log_data_port": 59000
		},
		"host_net_info" : {
			"cmd_data_ip" : "${HOSTIP}",
			"cmd_data_port": 56000,
			"push_msg_ip": "",
			"push_msg_port": 0,
			"point_data_ip": "${HOSTIP}",
			"point_data_port": 57000,
			"imu_data_ip" : "${HOSTIP}",
			"imu_data_port": 58000,
			"log_data_ip" : "",
			"log_data_port": 59000
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