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

#include "qPID.h"
#include "qGlobals.h"






QPIDClass::QPIDClass()
{
}

void QPIDClass::begin()
{	

}


int QPIDClass::DoControlAlgorithm(int setPoint, int currentTemp)
{
	float Output;
	float error = setPoint - currentTemp;
					
	if ((error < 0 && prevError > 0) || (error > 0 && prevError < outMax))
			{
				integralSum += (PID_I * error);			
				if (integralSum> outMax) integralSum = outMax;
				else if (integralSum< outMin) integralSum = outMin;
			}

	//float dInput = (curProbes[pitProbeIndex].runningAvg - currentTemp);
	float dInput = (setPoint - currentTemp);

	//*Compute PID Output
	Output = (PID_P * error) + (integralSum) + (PID_D * dInput);

	if (Output > outMax) Output = outMax;
		else if (Output < outMin) Output = outMin;

	//Remember some variables for next time
	prevError = Output;			
			
	return Output;
}

void QPIDClass::reset()
{
	prevError = 0; integralSum = 0;
}


void QPIDClass::AsJson(char *jData)
{
	char pidp[15],pidi[15],pidd[15],pidb[15];  //floatstr

	fBuf(jData, 300, F("{p:%s,i:%s,d:%s,b:%s,f:%i}"),
		dtostrf(PID_P,0,6,pidp), dtostrf(PID_I,0,6, pidi), dtostrf(PID_D,0,6,pidd), dtostrf(PID_BIAS,0,6,pidb),PID_I_FREQ);
}

void QPIDClass::SetValuesJson(const String& msgStr)
{
	String curPart, curField, curVal;
	for (int fy = 0; fy <= 50; fy++)
	{
		curPart = String(getValue(msgStr, fy, ','));  //grab first node separated by ,
		if (curPart == "") { break; } //exit if not found
		curField = getValue(curPart, 0, ':');
		curVal = getValue(curPart, 1, ':');		
		if (curField == "p") { PID_P = curVal.toFloat(); }
		else if (curField == "i") { PID_I = curVal.toFloat(); }
		else if (curField == "d") { PID_D = curVal.toFloat(); }
		else if (curField == "b") { PID_BIAS = curVal.toFloat(); }
		else if (curField == "f") { PID_I_FREQ = curVal.toInt(); }		
	}
		
}


