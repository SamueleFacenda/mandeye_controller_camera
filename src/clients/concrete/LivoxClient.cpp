#include "clients/concrete/LivoxClient.h"
#include <livox_lidar_api.h>
#include <livox_lidar_def.h>
#include "utils/save_laz.h"
#include "livox_types.h"
#include <iostream>
#include <thread>
#include <fstream>

namespace mandeye
{

LivoxClient::LivoxClient() : imuIteratorToFileSaver("csv", "imu", [](const LivoxIMU& imu) {
	std::stringstream ss;
	ss << imu.timestamp << " " << imu.point.gyro_x << " " << imu.point.gyro_y << " " << imu.point.gyro_z << " " << imu.point.acc_x << " "
	   << imu.point.acc_y << " " << imu.point.acc_z << " " << imu.laser_id;
	return ss.str();
}), lidarIteratorToFileSaver("lidar", "ls", [](const std::pair<const uint32_t, std::string>& id_sn) {
	return std::to_string(id_sn.first) + " " + id_sn.second;
}) {
	m_timestamp = -1;
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

nlohmann::json LivoxClient::produceStatus()
{
	nlohmann::json data;
	data["init_success"] = init_succes;

	if (!m_LivoxLidarInfo.empty())
	{
		data["LivoxLidarInfo"]["dev_type"] = m_LivoxLidarInfo.begin()->second.dev_type;
		data["LivoxLidarInfo"]["lidar_ip"] = m_LivoxLidarInfo.begin()->second.lidar_ip;
		data["LivoxLidarInfo"]["sn"] = m_LivoxLidarInfo.begin()->second.sn;

		auto array = nlohmann::json::array();
		for (auto& it : m_LivoxLidarInfo)
		{
			nlohmann::json livoxData;
			livoxData["dev_type"] = it.second.dev_type;
			livoxData["lidar_ip"] = it.second.lidar_ip;
			livoxData["sn"] = it.second.sn;
			array.push_back(livoxData);
		}
		data["multi"]["LivoxLidarInfo"] = array;

	}
	else
	{
		data["LivoxLidarInfo"]["dev_type"] = "null";
		data["LivoxLidarInfo"]["lidar_ip"] = "null";
		data["LivoxLidarInfo"]["sn"] = "null";
	}
	if (!m_recivedImuMsgs.empty())
	{
		data["counters"]["imu"] = m_recivedImuMsgs.begin()->second;
		auto array = nlohmann::json::array();
		for (auto& it : m_recivedImuMsgs)
		{
			array.push_back(it.second);
		}
		data["multi"]["imu"] = array;
	}
	else
	{
		data["counters"]["imu"] = 0;
	}
	if(!m_recivedPointMessages.empty())
	{
		data["counters"]["lidar"] = m_recivedPointMessages.begin()->second;
		auto array = nlohmann::json::array();
		for (auto& it : m_recivedPointMessages)
		{
			array.push_back(it.second);
		}
		data["multi"]["lidar"] = array;
	}
	else
	{
		data["counters"]["lidar"] = 0;
	}

	std::lock_guard<std::mutex> lcK1(m_bufferLidarMutex);
	std::lock_guard<std::mutex> lcK2(m_bufferImuMutex);
	data["LivoxLidarInfo"]["timestamp"] = m_timestamp;


	auto arrayworkMode = nlohmann::json::array();
	for (auto& mode : m_LivoxLidarWorkMode)
	{
		std::string modeName;
		if (auto it = WorkModeToStr.find(mode.second); it != WorkModeToStr.end())
		{
			modeName = it->second;
		}
		else
		{
			modeName = "unknown";
		}
		std::stringstream oss;
		oss << mode.second << " " << modeName;
		arrayworkMode.push_back(oss.str());
	}
	data["multi"]["workmode"] = arrayworkMode;

	auto arrayTimeSync = nlohmann::json::array();
	for (auto& [_,mode] : m_LivoxLidarTimeSync)
	{
		arrayTimeSync.push_back(mode);
	}
	data["multi"]["timesyncmode"] = arrayTimeSync;

	auto array = nlohmann::json::array();
	for (auto& timestamp : m_handleToLastTimestamp)
	{
		array.push_back(timestamp.second);
	}
	data["multi"]["timestamps"] = array;


	auto arraysn = nlohmann::json::array();
	for (auto& sn : m_serialNumbers)
	{
		arraysn.push_back(sn);
	}
	data["multi"]["sn"] = arraysn;

	if(m_bufferLivoxPtr)
	{
		data["buffers"]["point"]["counter"] = m_bufferLivoxPtr->size();
	}
	else
	{
		data["buffers"]["point"]["counter"] = "NULL";
	}
	if(m_bufferIMUPtr)
	{
		data["buffers"]["IMU"]["counter"] = m_bufferIMUPtr->size();
	}
	else
	{
		data["buffers"]["IMU"]["counter"] = "NULL";
	}
	return data;
}

void LivoxClient::setTimestamp(uint64_t ts, LivoxClient* this_ptr)
{
	std::lock_guard<std::mutex> lcK(this_ptr->m_timestampMutex);
	if (this_ptr->m_timestamp == -1) {
		this_ptr->systemTimestampDelay = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - ts;
	}
	this_ptr->m_timestamp = ts + this_ptr->systemTimestampDelay;
}

void LivoxClient::startLog()
{
	std::lock_guard<std::mutex> lcK1(m_bufferLidarMutex);
	std::lock_guard<std::mutex> lcK2(m_bufferImuMutex);
	m_bufferLivoxPtr = std::make_shared<LivoxPointsBuffer>();
	m_bufferIMUPtr = std::make_shared<LivoxIMUBuffer>();
}

void LivoxClient::stopLog()
{
	std::lock_guard<std::mutex> lcK1(m_bufferLidarMutex);
	std::lock_guard<std::mutex> lcK2(m_bufferImuMutex);
	m_bufferLivoxPtr = nullptr;
	m_bufferIMUPtr = nullptr;
}

std::pair<LivoxPointsBufferPtr, LivoxIMUBufferPtr> LivoxClient::retrieveData()
{
	std::lock_guard<std::mutex> lck1(m_bufferLidarMutex);
	std::lock_guard<std::mutex> lck2(m_bufferImuMutex);
	LivoxPointsBufferPtr returnPointerLidar{std::make_shared<LivoxPointsBuffer>()};
	LivoxIMUBufferPtr returnPointerImu{std::make_shared<LivoxIMUBuffer>()};
	std::swap(m_bufferIMUPtr, returnPointerImu);
	std::swap(m_bufferLivoxPtr, returnPointerLidar);
	return std::pair{returnPointerLidar, returnPointerImu};
}
void LivoxClient::testThread()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	std::cout << "Livox periodical watch thread" << std::endl;
	while(!isDone)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));


		std::lock_guard<std::mutex> lcK(this->m_lidarInfoMutex);
		for (auto& it : this->m_handleToSerialNumber)
		{
			const auto & handle = it.first;
			QueryLivoxLidarInternalInfo(handle, &LivoxClient::QueryInternalInfoCallback, this);

			// wakey wakey sleepy head - if lidar is sleeping, wake it up
			if (auto it = m_LivoxLidarWorkMode.find(handle); it != m_LivoxLidarWorkMode.end())
			{
				if (it->second != kLivoxLidarNormal)
				{
					std::cout << "wakey wakey lidar with handle" << it->first << std::endl;
					SetLivoxLidarWorkMode(handle, kLivoxLidarNormal, &LivoxClient::WorkModeCallback, this);
				}
			}
		}

	}
}
bool LivoxClient::startListener(const std::string& interfaceIp)
{
	constexpr char configFn[] = "/tmp/config.json";

	std::string fillInConfig(LivoxClient::config);
	fillInConfig = ReplaceAll(fillInConfig, "${HOSTIP}", interfaceIp);

	std::ofstream configFile(configFn);
	configFile << fillInConfig;
	configFile.close();
	DisableLivoxSdkConsoleLogger();
	init_succes = LivoxLidarSdkInit(configFn);
	if(!init_succes)
	{
		return false;
	}
	SetLivoxLidarPointCloudCallBack(PointCloudCallback, (void*)this);
	SetLivoxLidarImuDataCallback(ImuDataCallback, (void*)this);
	SetLivoxLidarInfoChangeCallback(LidarInfoChangeCallback, (void*)this);

	m_livoxWatchThread = std::thread(&LivoxClient::testThread, this);
	return true;
}

union ToUint64
{
	uint64_t data;
	uint8_t array[8];
};

void LivoxClient::PointCloudCallback(uint32_t handle,
									 const uint8_t dev_type,
									 LivoxLidarEthernetPacket* data,
									 void* client_data)
{
	if(data == nullptr || client_data == nullptr)
	{
		return;
	}

	LivoxClient* this_ptr = (LivoxClient*)client_data;

	this_ptr->m_recivedPointMessages[handle]++;
	const auto laser_id = this_ptr->handleToLidarId(handle);
	//  printf("point cloud handle: %u, data_num: %d, data_type: %d, length: %d, frame_counter: %d\n",
	//         handle, data->dot_num, data->data_type, data->length, data->frame_cnt);

	if(data->data_type == kLivoxLidarCartesianCoordinateHighData)
	{
		LivoxLidarCartesianHighRawPoint* p_point_data =
			(LivoxLidarCartesianHighRawPoint*)data->data;
		std::lock_guard<std::mutex> lcK(this_ptr->m_bufferLidarMutex);
		ToUint64 toUint64;
		std::memcpy(toUint64.array, data->timestamp, sizeof(uint64_t));
		setTimestamp(toUint64.data, this_ptr);
		this_ptr->m_handleToLastTimestamp[handle] = toUint64.data + this_ptr->systemTimestampDelay;

		if(this_ptr->m_bufferLivoxPtr == nullptr)
		{
			return;
		}
		auto& buffer = this_ptr->m_bufferLivoxPtr;
		//buffer->resize(buffer->size() + data->dot_num);
		for(uint32_t i = 0; i < data->dot_num; i++)
		{
			LivoxPoint point;
			point.point = p_point_data[i];
			point.laser_id = laser_id;
			point.timestamp = toUint64.data + i * data->time_interval + this_ptr->systemTimestampDelay;
			if(point.timestamp > 0){
				buffer->push_back(point);
			}
		}
	}
	else if(data->data_type == kLivoxLidarCartesianCoordinateLowData)
	{
		LivoxLidarCartesianLowRawPoint* p_point_data = (LivoxLidarCartesianLowRawPoint*)data->data;
	}
	else if(data->data_type == kLivoxLidarSphericalCoordinateData)
	{
		LivoxLidarSpherPoint* p_point_data = (LivoxLidarSpherPoint*)data->data;
	}
}

void LivoxClient::ImuDataCallback(uint32_t handle,
								  const uint8_t dev_type,
								  LivoxLidarEthernetPacket* data,
								  void* client_data)
{
	if(data == nullptr || client_data == nullptr)
	{
		return;
	}

	LivoxClient* this_ptr = (LivoxClient*)client_data;
	if(data->data_type == kLivoxLidarImuData)
	{
		const auto laser_id = this_ptr->handleToLidarId(handle);
		this_ptr->m_recivedImuMsgs[handle]++;
		LivoxLidarImuRawPoint* p_imu_data = (LivoxLidarImuRawPoint*)data->data;
		std::lock_guard<std::mutex> lcK(this_ptr->m_bufferImuMutex);
		ToUint64 toUint64;
		std::memcpy(toUint64.array, data->timestamp, sizeof(uint64_t));
		// setTimestamp(toUint64.data, this_ptr); // sync timestamp only with pointclouds
		if(this_ptr->m_bufferIMUPtr == nullptr)
		{
			return;
		}
		auto& buffer = this_ptr->m_bufferIMUPtr;
		//buffer->resize(buffer->size() + 1);
		LivoxIMU point;
		point.point = *p_imu_data;
		point.timestamp = toUint64.data + this_ptr->systemTimestampDelay;
		point.laser_id = laser_id;
		if(point.timestamp > 0){
			buffer->push_back(point);
		}
	}
}

void LivoxClient::WorkModeCallback(livox_status status,
								   uint32_t handle,
								   LivoxLidarAsyncControlResponse* response,
								   void* client_data)
{
	if(response == nullptr)
	{
		return;
	}
	printf("WorkModeCallack, status:%u, handle:%u, ret_code:%u, error_key:%u",
		   status,
		   handle,
		   response->ret_code,
		   response->error_key);
}

void LivoxClient::RebootCallback(livox_status status,
								 uint32_t handle,
								 LivoxLidarRebootResponse* response,
								 void* client_data)
{
	if(response == nullptr)
	{
		return;
	}
	printf("RebootCallback, status:%u, handle:%u, ret_code:%u", status, handle, response->ret_code);
}

void LivoxClient::SetIpInfoCallback(livox_status status,
									uint32_t handle,
									LivoxLidarAsyncControlResponse* response,
									void* client_data)
{
	if(response == nullptr)
	{
		return;
	}
	printf("LivoxLidarIpInfoCallback, status:%u, handle:%u, ret_code:%u, error_key:%u",
		   status,
		   handle,
		   response->ret_code,
		   response->error_key);

	if(response->ret_code == 0 && response->error_key == 0)
	{
		LivoxLidarRequestReboot(handle, RebootCallback, nullptr);
	}
}

void LivoxClient::QueryInternalInfoCallback(livox_status status,
											uint32_t handle,
											LivoxLidarDiagInternalInfoResponse* response,
											void* client_data)
{
	if(status != kLivoxLidarStatusSuccess)
	{
		printf("Query lidar internal info failed.\n");
		LivoxClient* this_ptr = (LivoxClient*)(client_data);
		assert(this_ptr);
		std::lock_guard<std::mutex> lcK(this_ptr->m_lidarInfoMutex);
		this_ptr->m_LivoxLidarWorkMode[handle] = -1;
		return;
	}

	if(response == nullptr)
	{
		return;
	}

	uint8_t host_point_ipaddr[4]{0};
	uint16_t host_point_port = 0;
	uint16_t lidar_point_port = 0;

	uint8_t host_imu_ipaddr[4]{0};
	uint16_t host_imu_data_port = 0;
	uint16_t lidar_imu_data_port = 0;

	uint16_t off = 0;
	for(uint8_t i = 0; i < response->param_num; ++i)
	{
		LivoxLidarKeyValueParam* kv = (LivoxLidarKeyValueParam*)&response->data[off];
		if(kv->key == kKeyLidarPointDataHostIPCfg)
		{
			memcpy(host_point_ipaddr, &(kv->value[0]), sizeof(uint8_t) * 4);
			memcpy(&(host_point_port), &(kv->value[4]), sizeof(uint16_t));
			memcpy(&(lidar_point_port), &(kv->value[6]), sizeof(uint16_t));
		}
		else if(kv->key == kKeyLidarImuHostIPCfg)
		{
			memcpy(host_imu_ipaddr, &(kv->value[0]), sizeof(uint8_t) * 4);
			memcpy(&(host_imu_data_port), &(kv->value[4]), sizeof(uint16_t));
			memcpy(&(lidar_imu_data_port), &(kv->value[6]), sizeof(uint16_t));
		}
		else if (kv->key == kKeyWorkMode)
		{
			LivoxClient* this_ptr = (LivoxClient*)(client_data);
			assert(this_ptr);
			std::lock_guard<std::mutex> lcK(this_ptr->m_lidarInfoMutex);
			this_ptr->m_LivoxLidarWorkMode[handle] = static_cast<LivoxLidarWorkMode>(kv->value[0]);
		}
		else if (kv->key == kKeyTimeSyncType)
		{
			LivoxClient* this_ptr = (LivoxClient*)(client_data);
			assert(this_ptr);
			std::lock_guard<std::mutex> lcK(this_ptr->m_lidarInfoMutex);
			this_ptr->m_LivoxLidarTimeSync[handle] = static_cast<uint8_t>(kv->value[0]);
		}
		off += sizeof(uint16_t) * 2;
		off += kv->length;
	}

	return; //disable annoying logging
	printf("Host point cloud ip addr:%u.%u.%u.%u, host point cloud port:%u, lidar point cloud "
		   "port:%u.\n",
		   host_point_ipaddr[0],
		   host_point_ipaddr[1],
		   host_point_ipaddr[2],
		   host_point_ipaddr[3],
		   host_point_port,
		   lidar_point_port);

	printf("Host imu ip addr:%u.%u.%u.%u, host imu port:%u, lidar imu port:%u.\n",
		   host_imu_ipaddr[0],
		   host_imu_ipaddr[1],
		   host_imu_ipaddr[2],
		   host_imu_ipaddr[3],
		   host_imu_data_port,
		   lidar_imu_data_port);
}

void LivoxClient::LidarInfoChangeCallback(const uint32_t handle,
										  const LivoxLidarInfo* info,
										  void* client_data)
{
	if(info == nullptr)
	{
		printf("lidar info change callback failed, the info is nullptr.\n");
		return;
	}
	printf("LidarInfoChangeCallback Lidar handle: %u SN: %s\n", handle, info->sn);
	assert(client_data);
	SetLivoxLidarWorkMode(handle, kLivoxLidarNormal, &LivoxClient::WorkModeCallback, client_data);

	QueryLivoxLidarInternalInfo(handle, &LivoxClient::QueryInternalInfoCallback, client_data);
	LivoxClient* this_ptr = (LivoxClient*)(client_data);
	if(this_ptr)
	{
		std::lock_guard<std::mutex> lcK(this_ptr->m_lidarInfoMutex);
		this_ptr->m_LivoxLidarInfo[handle] = *info;
		this_ptr->m_recivedImuMsgs[handle] = 0;
		this_ptr->m_recivedPointMessages[handle] = 0;
		this_ptr->m_handleToLastTimestamp[handle] = 0;
		const std::string sn(info->sn);
		this_ptr->m_handleToSerialNumber[handle] = sn;
		this_ptr->m_serialNumbers.insert(sn);
		std::cout << " **** Adding lidar " <<sn << " handle " << handle << std::endl;
	}
}
uint64_t LivoxClient::getTimestamp()
{
	std::lock_guard<std::mutex> lcK(m_timestampMutex);
	return m_timestamp;
}

std::unordered_map<uint32_t, std::string> LivoxClient::getSerialNumberToLidarIdMapping() const
{
	std::lock_guard<std::mutex> lcK(m_lidarInfoMutex);
	std::unordered_map<uint32_t, std::string> ret;
	for (auto& [handle, info] : m_LivoxLidarInfo)
	{
		uint16_t id = handleToLidarId(handle);
		ret[id] = info.sn;
	}
	return ret;
}

uint16_t LivoxClient::handleToLidarId(uint32_t handle) const
{
	auto it = m_handleToSerialNumber.find(handle);
	if(it != m_handleToSerialNumber.end())
	{
		const auto &sn = it->second;
		auto it2 = m_serialNumbers.find(sn);
		return std::distance(m_serialNumbers.begin(), it2);
	}

	return 255;
}

void LivoxClient::saveDumpedChunkToDirectory(const std::filesystem::path& directory, int chunk) {
	imuIteratorToFileSaver.saveDumpedChunkToDirectory(directory, chunk);
	// lidarIteratorToFileSaver.setBuffer(dumpedSerialNumberToLidarIdMapping.begin(), dumpedSerialNumberToLidarIdMapping.end());
	lidarIteratorToFileSaver.saveDumpedChunkToDirectory(directory, chunk);

	char pointcloudFileName[64];
	snprintf(pointcloudFileName, 64, "lidar%04d.laz", chunk);
	std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(pointcloudFileName);
	std::cout << "Savig lidar buffer of size " << dumpedBufferLivoxPtr->size() << " to " << lidarFilePath << std::endl;
	saveLaz(lidarFilePath.string(), dumpedBufferLivoxPtr);
}

void LivoxClient::dumpChunkInternally() {
	auto [lidarBuffer, imuBuffer] = retrieveData();
	auto lidarList = getSerialNumberToLidarIdMapping();

	lidarIteratorToFileSaver.setBuffer(lidarList);
	imuIteratorToFileSaver.setBuffer(*imuBuffer);
	dumpedBufferLivoxPtr = lidarBuffer;
	dumpedBufferImuPtr = imuBuffer; // needed to avoid garbage collection
}

std::string LivoxClient::getJsonName()
{
	return "livox";
}

} // namespace mandeye