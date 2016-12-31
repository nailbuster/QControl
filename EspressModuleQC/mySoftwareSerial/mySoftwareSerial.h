/*
mySoftwareSerial.h

SoftwareSerial.cpp - Implementation of the Arduino software serial for ESP8266.
Copyright (c) 2015-2016 Peter Lerup. All rights reserved.

small changes by David@nailbuster.com  :  added DEFER option so that create can use 'soft-pin-assignment'.  If you call defer(true) when
defining then you can set the rx/tx pins variable and then call on your begin
mySerialSetup(int receivePin, int transmitPin, bool inverse_logic = false, unsigned int buffSize = 64);

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA



*/

#ifndef _MYSOFTWARESERIAL_h
#define _MYSOFTWARESERIAL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#include <inttypes.h>
#include <Stream.h>


// This class is compatible with the corresponding AVR one,
// the constructor however has an optional rx buffer size.
// Speed up to 115200 can be used.


class SoftwareSerial : public Stream
{
public:
	SoftwareSerial(int receivePin, int transmitPin, bool inverse_logic = false, unsigned int buffSize = 64);
	SoftwareSerial(bool defer);       //dp will cause defer of startup
	~SoftwareSerial();

	void mySerialSetup(int receivePin, int transmitPin, bool inverse_logic = false, unsigned int buffSize = 64);     //dp  dynamic setup instead of on create.....

	void begin(long speed);
	void setTransmitEnablePin(int transmitEnablePin);
	
	int peek();

	virtual size_t write(uint8_t byte);
	virtual int read();
	virtual int available();
	virtual void flush();
	operator bool() { return m_rxValid || m_txValid; }

	// Disable or enable interrupts on the rx pin
	void enableRx(bool on);

	void rxRead();

	using Print::write;

private:
	bool isValidGPIOpin(int pin);

	// Member variables
	int m_rxPin, m_txPin, m_txEnablePin;
	bool m_rxValid, m_txValid, m_txEnableValid;
	bool m_invert;
	unsigned long m_bitTime;
	unsigned int m_inPos, m_outPos;
	int m_buffSize;
	uint8_t *m_buffer;

};

// If only one tx or rx wanted then use this as parameter for the unused pin
#define SW_SERIAL_UNUSED_PIN -1

#endif

