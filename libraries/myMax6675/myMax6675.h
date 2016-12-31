// myMax6675.h

#ifndef _MYMAX6675_h
#define _MYMAX6675_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#define MAX_USE_SPI_TRANSACTION   //****set when other devices are on the SPI bus!!!


class MyMax6675Class
{
 protected:


 public:
	MyMax6675Class();
	void begin(uint8_t CSPin);
	double readTemperatureF();
	double readTemperatureC();
	bool isConnected = false;
private:
	uint8_t _cs;

};

extern MyMax6675Class MyMax6675;

#endif

