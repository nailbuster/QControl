// 
// 
// 
#ifdef __AVR
#include <avr/pgmspace.h>
#elif defined(ESP8266)
#include <pgmspace.h>
#define _delay_ms(ms) delayMicroseconds((ms) * 1000)
#else defined(MAPLEMINI)
//#include <pgmspace.h>
#define _delay_ms(ms) delayMicroseconds((ms) * 1000)
#endif

#include "myMax6675.h"
#include <SPI.h>




MyMax6675Class MyMax6675;



#ifdef MAX_USE_SPI_TRANSACTION
//esp likes MODE0,  arduino likes MODE1 ????

SPISettings MaxspiSettings = SPISettings(1000000, MSBFIRST, SPI_MODE0);

static inline void spi_begin(void) __attribute__((always_inline));
static inline void spi_begin(void) {
	SPI.beginTransaction(MaxspiSettings);
}
static inline void spi_end(void) __attribute__((always_inline));
static inline void spi_end(void) {
	SPI.endTransaction();
}
#else
#define spi_begin()
#define spi_end()
#endif

MyMax6675Class::MyMax6675Class()
{
}

void MyMax6675Class::begin(uint8_t CSPin)
{
	_cs = CSPin;
	pinMode(_cs, OUTPUT);
	SPI.begin();
	digitalWrite(_cs, HIGH);
}

double MyMax6675Class::readTemperatureC()
{
	isConnected = false;  //defaul
	uint16_t v= 65535;

	spi_begin();

	digitalWrite(_cs, LOW);
	_delay_ms(5);

	
	v = SPI.transfer(0);
	v <<= 8;
	v |= SPI.transfer(0); 
	
	

	
	digitalWrite(_cs, HIGH);

	spi_end();

	if (v & 0x8000) return NAN;  //error in resonse bit 15 is always 0;

	if (v & 0x4) {  //check bit 2 to see if thermo connected;
		// uh oh, no thermocouple attached!		
		return NAN;
		//return -100;
	} else isConnected = true;

	v >>= 3;
	return v*0.25;
}

double MyMax6675Class::readTemperatureF()
{
	return readTemperatureC() * 9.0 / 5.0 + 32;
}
