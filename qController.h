/*
QControl PID controller for BBQ

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

#ifndef _QCONTROLLER_h
#define _QCONTROLLER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif



class QControllerClass
{
 protected:
	 unsigned long lastTimerChk;	 

 public:
	//void init();
	 QControllerClass();
	 byte EVersion = 1;    //eeprom version
	 unsigned int setpoint = 200;   //
	 byte Status = 0;  // status=0 firstrun
	 unsigned int timestart;  //  
	 unsigned int totalseconds = 0;
	 bool NoPitt = false;   //if pitprobe is offline....
	 int testpoint = 0;	 
	 bool LidOpened = false;
	 int  LidOpenCountdown = 0;
	 bool preHeat = false;
	 bool debugMode = false;
	 bool noConfig = false;  //no configuration found...
	 bool targetReached = false;
	 byte lidOpenOffset = 6;
	 byte preHeatOffset = 50;  //within 50 degrees it will not preheating
	 int lidOpenTime = 30;
	  
	 int preHeatMaxSpeed = 100;  //percent of max speed during preheat
	 int preHeatTemp = 80;  //percent of settemp so 80 is 80% of settemp
	 int MinTempFan = 0; //alarm if lower than 20 from setpoint (after reached)
	 int MaxSetPoint = 600; //alarm if higher than 20 from setpoint (after reached)

	 int msReadInterval = 1000; //how often to read temps and set fan/PID etc...	 
	 unsigned int pit_temp = 10;
	// double qatemp = 100;   //testing qa temperature.... should be rem



	 void ReadConfig();
	 void writeConfig(bool AllData);
	 void Begin();
	 void Run();
	 void StartCook();
	 void StopCook();
	 void AsJson(char *jData);
	 void SetValuesJson(const String& msgStr);
	 void CheckAlarms();
	 void LinkSetTemp(int NewTemp);  //in F	 

};

//extern QControllerClass curQ;  //Main Controller

#endif

