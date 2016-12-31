/*
QControl ESP8266 Connection

Copyright (c) 2016 David Paiva (david@nailbuster.com). All rights reserved.

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

#ifndef _GLOBALS_h
#define _GLOBALS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#define comdelay 10  //serial comdelay
//#define AVR_COM_BAUDRATE 57600

class GlobalsClass
{
public:
	 byte rxpin = 4;
	 byte txpin = 5;
	 byte resetpin = 2;
	 int buffsize = 128;
	 long baudrate = 38400;
	 long baudrateFlash = 57600;

 String  avrSetPoint,
		 avrPitTemp,
		 avrFood1,
		 avrFood2,
		 avrAmbient,
		 avrFan,
		 avrFanMovAvg,
		 avrLidOpenCountdown;
 int ResetAlarmSeconds = 10;  //number of seconds before we reset alarm....0 = means never....
 unsigned long ResetTimeCheck = 0; 
 GlobalsClass();
 void SetTemp(int sndTemp);
 void begin();
 void SendHeatGeneralToAVR(String fname);
 void SendProbesToAVR(String fname);
 bool WaitForAVROK();
 void SendStringToAVR(const String& msgStr);
 void handle();
 void checkSerialMsg();
 void ResetAlarms();
 void loadSetup();
 void ConfigAlarms(String msgStr);
 String getValue(String data, int index, char separator = ',');
};

extern GlobalsClass avrGlobal;

#endif

