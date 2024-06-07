#include "clients/concrete/ImuFileSaver.h"

namespace mandeye_utils
{

void ImuFileSaver::setBuffer(const mandeye::LivoxIMUBufferPtr& inBuf) {
	buffer = inBuf;
}

void ImuFileSaver::printBufferToFileString(std::stringstream& fss) {
	for(const auto& p : *buffer)
	{
		if(p.timestamp > 0)
		{
			fss << p.timestamp << " " << p.point.gyro_x << " " << p.point.gyro_y << " " << p.point.gyro_z << " " << p.point.acc_x << " "
			   << p.point.acc_y << " " << p.point.acc_z << " " << p.laser_id << std::endl;
		}
	}
}

std::string ImuFileSaver::getFileExtension()
{
	return "csv";
}

std::string ImuFileSaver::getFileIdentifier()
{
	return "imu";
}
} // namespace mandeye_utils