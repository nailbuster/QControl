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

#ifndef _QFAN_h
#define _QFAN_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <Servo.h>

class QFanClass
{
 protected:
	 unsigned long RelayStartTime;
	 byte gpioPin = 3;   //fan pwm pin
	 byte gpioServoPIN = 6;  //servo pwm pin
	 Servo myServo;  //servo...
	 byte fanStatus = 0;
	 byte fanHasPWM = 1;
	 int curDuty = 0;   //0-255 (maxPWM)%;
	 int hardwarePWM = 255;  //defaults to 255, some pwm can go to 1024 use ratio
	 int maxSpeed = 100;  //max % fan 0-100...
	 int minSpeed = 0;    //min % fan
	 int relaytime = 10000;  //ms of relaytiming when in pulsemode or nopwm 10s default;	
	 byte interval = 3;
	 byte maxTime = 3;
	 byte maxStart = 100; //when starting up, max fan speed % 
	 int ServoLow = 90;   //degree 0-180
	 int ServoHi = 150;   //degree 0-180
	 byte curServoPos = 255;  //0-180 servo pos;  255 will force startup reset....
	 unsigned long ServoLastTimePos = 0;  //last time moved servo;
	 byte ServoTimeRelease = 3; //release after 3 seconds;
	 bool ServoRound = true;  //round movements to avoid hunting
	 float runningServoAvg = 0;
	 int pitMin = 0;   //don't start fan until pit is at least this temperature in f	

	 float runningAvg = 90;
	
	 byte fanStartDuty = 0;   // when fan starts and servo is at max(servoHI); 0 means noServo

 public:
	
	QFanClass();  //create class
	


	void SetFanSpeed(byte fanSpeed);
	void SetServoPos();   //will set Servo Position based on CurDuty...called by setfanspeed
	void begin();
	void SetPWMFreq(int FreqIndx);
	int duty100(bool avg);  //returns 0-100% for display....	

	void AsJson(char *jData);
	void SetValuesJson(const String& msgStr);
};




#endif

