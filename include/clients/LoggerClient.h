#ifndef MANDEYE_MULTISENSOR_LOGGERCLIENT_H
#define MANDEYE_MULTISENSOR_LOGGERCLIENT_H

namespace mandeye_utils {

class LoggerClient {
public:
	virtual void startLog() = 0;
	virtual void stopLog() = 0;
};

}

#endif //MANDEYE_MULTISENSOR_LOGGERCLIENT_H
