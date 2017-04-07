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


//#define AVR_COM_BAUDRATE 57600
#define MAX_PROBES 4
//#define HEATERMETER


#define vernum "1.0a"

#ifdef HEATERMETER
#define comdelay 200  //serial comdelay
#else 
#define comdelay 20  //serial comdelay
#endif // !HEATERMETER




#include "CookClass.h"

struct ProbesStruct {
	String Name;
	int AlarmLo, AlarmHi;
	int curTemp;
};


struct curDataStruct {
	unsigned long readTime = 0;
	int SetPoint = 0,
		PitTemp = 0,
		Food1 = 0,
		Food2 = 0,
		Food3 = 0,
		Fan = 0,
		FanAvg = 0,
		LidCountDown = 0;
};


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

 String AlarmRinging[4];

 ProbesStruct Probes[MAX_PROBES];
 curDataStruct curData;

  int ResetAlarmSeconds = 10;  //number of seconds before we reset alarm....0 = means never....
 byte updateInterval = 3;  //interval to poll AVR.
 unsigned long ResetTimeCheck, lastTimerChk;
 String AVRStringAsync = "";  //set this for async sending to AVR
 String FlashHMFileAsync = "";
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
 void loadProbes();
 void SaveProbes();
 void ConfigAlarms(String msgStr, bool Async = false);
 String getValue(String data, int index, char separator = ',');
 String getAlarmsJson(); 
};

class HMGlobalClass : public GlobalsClass
{
public:
	void SetTemp(int sndTemp);
	void SendHeatGeneralToAVR(String fname);
	void SendProbesToAVR(String fname);
	void ConfigAlarms(String msgStr, bool Async = false);
	void ResetAlarms();
};


#ifdef HEATERMETER
extern HMGlobalClass avrGlobal;
#else
extern GlobalsClass avrGlobal;
#endif

#endif

