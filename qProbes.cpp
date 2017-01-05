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

#include "qProbes.h"
#include "qGlobals.h"






float QProbeClass::analogReadFilter()
{
	int NUM_READS = 30;

	// read multiple values and sort them to take the mode
	int sortedValues[NUM_READS];
	for (int i = 0; i<NUM_READS; i++) {
		int value = analogRead(gpioPin);
		delay(1);
		int j;
		if (value<sortedValues[0] || i == 0) {
			j = 0; //insert at first position
		}
		else {
			for (j = 1; j<i; j++) {
				if (sortedValues[j - 1] <= value && sortedValues[j] >= value) {
					// j is insert position
					break;
				}
			}
		}
		for (int k = i; k>j; k--) {
			// move all values higher than current reading up one position
			sortedValues[k] = sortedValues[k - 1];
		}
		sortedValues[j] = value; //insert current reading
	}
	//return scaled mode of 10 values
	float returnval = 0;
	for (int i = NUM_READS / 2 - 5; i<(NUM_READS / 2 + 5); i++) {
		returnval += sortedValues[i];
	}
	returnval = returnval / 10;
	return returnval;
}



QProbeClass::QProbeClass()   
{
}

int QProbeClass::ReadTemperature()
{
	
	
	if (ProbeType == ptAnalog)
	{
		double R, T;
		int ADCValue;

		curADC = analogReadFilter();  //read mode filter 30 times;
		ADCValue = curADC;  //read	
		if (ADCValue > 1015)  //assume it's unplugged ??
			isConnected = false; else isConnected = true;

		R = log((1 / ((1024 / (double)ADCValue) - 1)) * (double)resistor);
		T = (COA)+(COB * R) + (COC * R * R * R);
		T = (1.0 / T) - 273.15;

		curTemp = (int)((T * 9.0 / 5.0) + 32.0);  //set probe in F
	} 
	else if (ProbeType == ptMax6675)
	{
		curTemp = maxSPI.readTemperatureF();
		isConnected = maxSPI.isConnected;
	}

	curTemp += ProbeFix;

	if (isConnected == false) { curTemp = -999; }

	//running avg.
	float alpha = 0.05; // factor to tune
	if (runningAvg < 0) runningAvg = curTemp; //first run tmp;
	runningAvg = (alpha * curTemp) + (1 - alpha) * runningAvg;	

	return curTemp;
}

void QProbeClass::begin()  //initialization
{
	if (ProbeType == ptMax6675)
	{
		maxSPI.begin(gpioPin);
		//Serial.println("pin" + String(gpioPin));
	} 
}




void QProbeClass::CheckAlarm()
{
	int alarmBuf = 10;  //degrees f to reset alarm hi/lo (so we don't repeat alarms when very close swings)
	if (isConnected == false) return;

	//check HI Alarm
	if (curTemp > AlarmMaxTemp && AlarmMaxTemp>0)
	{
		if (AlarmActiveMax == false)  //new alarm
		{
			//fire alarm high!
			curLink.SendAlarm(ProbeName, "High", AlarmMaxTemp, curTemp);
			AlarmActiveMax = true;
		}
	}
	else if (curTemp < (AlarmMaxTemp - alarmBuf)) { AlarmActiveMax = false; } //reset alarm if target reached

																			  //check Low Alarm

	if (curTemp < AlarmMinTemp && AlarmMinTemp > 0)
	{
		if (AlarmActiveMin == false)  //new alarm
		{
			//fire alarm low!
			curLink.SendAlarm(ProbeName, "Low", AlarmMinTemp, curTemp);
			AlarmActiveMin = true;
		}

	}
	else if (curTemp > (AlarmMinTemp + alarmBuf)) { AlarmActiveMin = false; } //reset alarm if target reached



}



void QProbeClass::AsJson(char *jData)
{
	char coa[15], cob[15], coc[15];

    fBuf(jData,300,F("{pn:\"%s\",pt:%i,ct:%i,mt:%i,gp:%i,aX:%i,aN:%i,a:%s,b:%s,c:%s,rs:%li,adc:%i,pf:%i,ic:%i}"),
            ProbeName, ProbeType, curTemp, maxTemp, gpioPin, AlarmMaxTemp, AlarmMinTemp, float2s(COA, coa, 7), float2s(COB, cob, 7), float2s(COC, coc, 7), resistor, curADC, ProbeFix, isConnected);
}




void QProbeClass::SetValuesJson(const String& msgStr)
{
	String curPart, curField, curVal;
	for (int fy = 0; fy <= 50; fy++)
	{		
		curPart = String(getValue(msgStr, fy, ','));  //grab first node separated by ,				
		if (curPart == "") { break; } //exit if not found
		curField = getValue(curPart, 0, ':');
		curVal = getValue(curPart, 1, ':');		
		if (curField == "pn") { curVal.remove(curVal.length() - 1, 1); curVal.remove(0, 1); curVal.toCharArray(ProbeName, 15, 0); } //remove quotes
		else if (curField == "pt") { ProbeType = curVal.toInt(); }
		else if (curField == "ct") { curTemp = curVal.toInt(); }
		else if (curField == "mt") { maxTemp = curVal.toInt(); }
		else if (curField == "gp") { gpioPin = curVal.toInt(); }
		else if (curField == "aX") { AlarmMaxTemp = curVal.toInt(); }
		else if (curField == "aN") { AlarmMinTemp = curVal.toInt(); }
		else if (curField == "a") { COA = curVal.toFloat(); }
		else if (curField == "b") { COB = curVal.toFloat(); }
		else if (curField == "c") { COC = curVal.toFloat(); }
		else if (curField == "rs") { resistor = curVal.toInt(); }
		else if (curField == "adc") { curADC = curVal.toInt(); }
		else if (curField == "pf") { ProbeFix = curVal.toInt(); }
		else if (curField == "ic") { isConnected = curVal.toInt(); }
	}

}
	

	






