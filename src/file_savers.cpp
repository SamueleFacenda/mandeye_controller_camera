#include "clients/LivoxClient.h"
#include "state_management.h"
#include "utils/save_laz.h"
#include "utils/utils.h"
#include <fstream>

namespace mandeye
{

using namespace std;

void savePointcloudData(const LivoxPointsBufferPtr& buffer, const string& directory, int chunk)
{
	char pointcloudFileName[256];
	snprintf(pointcloudFileName, 256, "lidar%04d.laz", chunk);
	filesystem::path lidarFilePath = filesystem::path(directory) / filesystem::path(pointcloudFileName);
	cout << "Savig lidar buffer of size " << buffer->size() << " to " << lidarFilePath << endl;
	saveLaz(lidarFilePath.string(), buffer);
}

void saveLidarList(const unordered_map<uint32_t, string>& lidars, const string& directory, int chunk)
{
	ofstream lidarStream;
	getSavingStream(lidarStream, directory, "sn", chunk);
	for(const auto& [id, sn] : lidars)
	{
		lidarStream << id << " " << sn << "\n";
	}
	lidarStream.close();
}

void saveImuData(const LivoxIMUBufferPtr& buffer, const string& directory, int chunk)
{
	ofstream imuFile;
	getSavingStream(imuFile, directory, "csv", chunk);
	stringstream ss;

	for(const auto& p : *buffer)
	{
		if(p.timestamp > 0)
		{
			ss << p.timestamp << " " << p.point.gyro_x << " " << p.point.gyro_y << " " << p.point.gyro_z << " " << p.point.acc_x << " "
			   << p.point.acc_y << " " << p.point.acc_z << " " << p.laser_id << endl;
		}
	}
	imuFile << ss.rdbuf();
	imuFile.close();
}

void saveGnssData(deque<string>& buffer, const string& directory, int chunk)
{
	ofstream gnssFile;
	getSavingStream(gnssFile, directory, "gnss", chunk);
	stringstream ss;

	for(const auto& p : buffer)
	{
		ss << p;
	}
	gnssFile << ss.rdbuf();
	gnssFile.close();
}

void getSavingStream(ofstream& out, const string& directory, const string& fileExtension, int chunkNumber)
{
	char filename[64];
	snprintf(filename, 64, "imu%04d.%s", chunkNumber, fileExtension.c_str());
	out.open(filename);
	if(out.fail())
	{
		cerr << "Error opening file '" << filename << "' !!" << endl;
		out.close();
		exit(1);
	}
}

bool saveChunkToDisk(const string& outDirectory, int chunk)
{
	mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, true);

	auto [lidarBuffer, imuBuffer] = livoxClientPtr->retrieveData();
	deque<string> gnssBuffer;
	if(gnssClientPtr)
	{
		gnssBuffer = gnssClientPtr->retrieveData();
	}

	if(outDirectory.empty())
	{
		app_state = States::USB_IO_ERROR;
		return false;
	}

	savePointcloudData(lidarBuffer, outDirectory, chunk);
	saveImuData(imuBuffer, outDirectory, chunk);
	auto lidarList = livoxClientPtr->getSerialNumberToLidarIdMapping();
	saveLidarList(lidarList, outDirectory, chunk);
	if(gnssClientPtr)
	{
		saveGnssData(gnssBuffer, outDirectory, chunk);
	}
	utils::syncDisk();

	mandeye::gpioClientPtr->setLed(mandeye::GpioClient::LED::LED_GPIO_COPY_DATA, false);

	return true;
}

} // namespace mandeye