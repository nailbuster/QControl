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

#ifndef _QPID_h
#define _QPID_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif



class QPIDClass
{
 protected:


 public:
	 QPIDClass();
	float PID_P = 5;
	float PID_I = 0.02;
	float PID_D = 0;
	float PID_BIAS = 3;
	int PID_I_FREQ = 6;
	byte outMax = 255;
	byte outMin = 0;
	float integralSum = 0, prevError = 0;
	unsigned char integralCount = 0;	
	void begin();
	int DoControlAlgorithm(int setPoint, int currentTemp);
	void reset();
	void AsJson(char *jData);
	void SetValuesJson(const String& msgStr);
};




#endif

