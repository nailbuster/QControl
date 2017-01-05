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

#ifndef _QPROBES_h
#define _QPROBES_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#include <myMax6675.h>

#define ptNone 0
#define ptAnalog 1
#define ptMax6675 2

class QProbeClass
{
 protected:
	 float analogReadFilter();
	 int ProbeType = 0;  //0=none  1=adc 2 = max6675	
	 char ProbeName[15];

	 float COA = 2.3067434E-4, COB = 2.3696596E-4, COC = 1.2636414E-7;
	 long resistor = 10000;
	 int curADC;
	 int maxTemp = 600;
	 int8_t ProbeFix = 0;  //add/minus temperature for probe errors....

	 MyMax6675Class maxSPI;  //if using max6675 temp probe;

 public:
	QProbeClass();
	boolean isConnected=false;
	int gpioPin = 1;

	int curTemp = 0;
	int AlarmMaxTemp, AlarmMinTemp;
	bool AlarmActiveMax = false;
	bool AlarmActiveMin = false;
	void CheckAlarm();

	float runningAvg=-1;

	int ReadTemperature();
	
	void begin();
	void AsJson(char *jData);
	void SetValuesJson(const String& msgStr);

};





#endif

