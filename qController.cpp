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

#include "qController.h"
#include "qGlobals.h"



template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
	const byte* p = (const byte*)(const void*)&value;
	unsigned int i;
	for (i = 0; i < sizeof(value); i++)
		EEPROM.write(ee++, *p++);
	return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
	byte* p = (byte*)(void*)&value;
	unsigned int i;
	for (i = 0; i < sizeof(value); i++)
		*p++ = EEPROM.read(ee++);
	return i;
}


void ReadEEPROMString(int &curPos,char eData[]) {
	//String tmp;
	char ch;
	int dp=0;

	eData[0] = '\0';		
	ch = EEPROM.read(curPos);
	curPos++;
	//Serial.print(ch);
	while (ch != '\0')
	{
		eData[dp] = ch;
		ch = EEPROM.read(curPos);
		curPos++;
		dp++;
		//Serial.print(char(ch));
	}	
	eData[dp] = '\0';  //null terminate
	
}


void WriteEEPROMString(int &curPos, char InData[]) {

	for (int i = 0; i < strlen(InData); i++)
	{
		EEPROM.write(curPos, InData[i]);
		curPos++;
	}
	EEPROM.write(curPos, '\0');  //null it
	curPos++;		
}





QControllerClass::QControllerClass()
{
}

void QControllerClass::ReadConfig()
{
	int curPos = 0;
	char chke;
	char jData[300];	
	curPos += EEPROM_readAnything(0, chke);  //good eprom with Q?            //write just quick when setpoint is changed....don't need to rewrite entire flash...
	//debug to reinit flash
	// chke='Z';
	if (chke != 'A') {		
		noConfig = true;		
	}
	else //good eeprom
	{

		curPos += EEPROM_readAnything(curPos, Status);  //good eprom with Q?  
		curPos += EEPROM_readAnything(curPos, setpoint);  //good eprom with Q?  
		curPos += EEPROM_readAnything(curPos, timestart);  //good eprom with Q?  
		curPos += EEPROM_readAnything(curPos, totalseconds);  //good eprom with Q? 

		ReadEEPROMString(curPos, jData);SetValuesJson(String (jData));
		ReadEEPROMString(curPos, jData);curPID.SetValuesJson(String (jData));
		ReadEEPROMString(curPos, jData);curFan.SetValuesJson(String (jData));



		for (int fx = 0; fx < MaxProbes; fx++)
		{
			ReadEEPROMString(curPos, jData);
			curProbes[fx].SetValuesJson(String (jData));
		}  //read to real curStatus                        

		noConfig = false;
	}

	
}

void QControllerClass::writeConfig(bool AllData){
int curPos = 0;
char chke = 'A';
char tmpChars[300];

curPos += EEPROM_writeAnything(0, chke);  //good eprom with Q?            //write just quick when setpoint is changed....don't need to rewrite entire flash...
curPos += EEPROM_writeAnything(curPos, Status);  //good eprom with Q?  
curPos += EEPROM_writeAnything(curPos, setpoint);  //good eprom with Q?  
curPos += EEPROM_writeAnything(curPos, timestart);  //good eprom with Q?  
curPos += EEPROM_writeAnything(curPos, totalseconds);  //good eprom with Q?  

if (AllData)
{
	AsJson(tmpChars);  WriteEEPROMString(curPos, tmpChars);  //Qcontroller object set    write jsons as null term
	curPID.AsJson(tmpChars); WriteEEPROMString(curPos, tmpChars);  //PID
	curFan.AsJson(tmpChars); WriteEEPROMString(curPos, tmpChars);  //FAN

	for (int fx = 0; fx<MaxProbes; fx++)
	{
		curProbes[fx].AsJson(tmpChars);
		WriteEEPROMString(curPos, tmpChars); //PROBES
	}  //read to real curStatus       
}
}


void QControllerClass::Begin()
{
	//EEPROM.write(0, 0); reset eeprom
	ReadConfig();  //load and setup variables
	if (noConfig) Serial.println(F("NEW"));
	curFan.begin();
	curPID.begin();
	curLink.begin();  //startup link	

 //debug must remove manually set the pins for now....
//curProbes[0].gpioPin = 2; curProbes[1].gpioPin = 3; curProbes[2].gpioPin = 4; curProbes[3].gpioPin = 5;  //my homemade thingy

//	curProbes[0].gpioPin = 13; curProbes[1].gpioPin = 14; curProbes[2].gpioPin = 15; curProbes[3].gpioPin = 5; // MEGA testing ramps board
//	curProbes[0].resistor = 4700; curProbes[1].resistor = 4700; curProbes[2].resistor = 4700;

//max testing
//	curProbes[0].gpioPin = 8; curProbes[1].gpioPin = 9; curProbes[2].gpioPin = 1; curProbes[3].gpioPin = 2; // MEGA testing ramps board
//	curProbes[0].ProbeType = 1; curProbes[1].ProbeType = 1;

//	curFan.minPWM = 150;
//	curFan.fanStartDuty = 200; //use servo upto 200
//	curFan.ServoLow = 85;
//	curFan.ServoHi = 180;
	//lidOpenTime = 60;


	for (int fx = 0; fx < MaxProbes; fx++)  //go through each probe and begin
	{
		curProbes[fx].begin();
	}


}

void QControllerClass::Run()
{	

	if ((millis() - lastTimerChk >= msReadInterval) && (curLink.inProgMode==false)) //process every interval (1 second for now)
	{
		lastTimerChk = millis();
		//char line[60];
		int PIDresult;

		for (int fx = 0; fx < MaxProbes; fx++)  //go through each probe and grab current temps in F
		{
			curProbes[fx].ReadTemperature();			
		}     //read all probes temperatures
		

		pit_temp = curProbes[pitProbeIndex].curTemp; //pitProbeIndex is pit probe;
		

		if (setpoint <= 0) setpoint = 1; //safety and division errors.
		
/*qa testing
		setpoint = 140;
		qatemp = (qatemp *1.0) + (((curFan.curDuty) / 255.0) * 4.0);
		if (curFan.curDuty > 0) qatemp += 1; else qatemp -= 0.30;
		pit_temp = int (qatemp);
		curProbes[pitProbeIndex].runningAvg = pit_temp; //so no lid detect....
*/

		if (LidOpened == false)
			{
				PIDresult = curPID.DoControlAlgorithm(setpoint, pit_temp);   //call PID
				curFan.SetFanSpeed(PIDresult);  //set fanspeed from PIDresult;  0-255....			
		    }
		else {
			curFan.SetFanSpeed(0);  //no fan during lidopenmode			
			//Serial.println("in lidopen");
		}


		if (abs(setpoint - pit_temp) <= 10) {   //target is reached within XX degrees of setpoint?
			targetReached = true;
		}

		// else targetReached = false; //for alarms and such.....
		//		}
		//else if (pit_temp<setpoint) curFan.SetFanSpeed(preHeatMaxSpeed/100 * 255); //when preHeat then use the preheat max speed...


		//saftey?
		if (pit_temp >(setpoint + 25)) curFan.SetFanSpeed(0);  //don't think...but who knows?				

		//lidopencheck timer
		if (LidOpened) {
			LidOpenCountdown -= 1;
			if (LidOpenCountdown <= 0) { LidOpened = false; }
		}

		//lidopen detect
		int lod = (curProbes[pitProbeIndex].runningAvg - pit_temp) * 100 / curProbes[pitProbeIndex].runningAvg;		
		if (lod >= lidOpenOffset && LidOpened==false && targetReached)
		{
		LidOpened = true;
		LidOpenCountdown = lidOpenTime;  //seconds to countdown		
		}

		CheckAlarms();  

	}  //main interval loop

	QGUI.run(); //run GUI object...does nothing yet...
	curLink.run();  //link object run
}

void QControllerClass::StartCook()
{
	Status = 1;
	writeConfig(false); //write status;
	preHeat = true;

}

void QControllerClass::StopCook()
{
	Status = 2;
	writeConfig(false); //write status;   
	preHeat = false;
}

void QControllerClass::AsJson(char *jData)
{
	
	fBuf(jData, 300, F("{lo:%i,lt:%i,ph:%i,pr:%i,aX:%i,aN:%i,ri:%i,pt:%i}"),
		lidOpenOffset, lidOpenTime, preHeatMaxSpeed, preHeatTemp, MaxSetPoint, MinTempFan, msReadInterval, pit_temp);

}

void QControllerClass::SetValuesJson(const String& msgStr)
{
	String curPart, curField, curVal;
	for (int fy = 0; fy <= 50; fy++)
	{
		curPart = String(getValue(msgStr, fy, ','));  //grab first node separated by ,
   	    if (curPart == "") { break; } //exit if nothing found
		curField = getValue(curPart, 0, ':');
		curVal = getValue(curPart, 1, ':');
		if (curField == "lo") { lidOpenOffset = curVal.toInt(); }
		else if (curField == "lt") { lidOpenTime = curVal.toInt(); }
		else if (curField == "ph") { preHeatMaxSpeed = curVal.toInt(); }
		else if (curField == "pr") { preHeatTemp = curVal.toInt(); }
		else if (curField == "aX") { MaxSetPoint = curVal.toInt(); }
		else if (curField == "aN") { MinTempFan = curVal.toInt(); }
	//	else if (curField == "ri") { msReadInterval = curVal.toInt(); }
		else if (curField == "pt") { pit_temp = curVal.toInt(); }
	}

		

}

void QControllerClass::CheckAlarms()

{
	int alarmBuf = 10;  //degrees to reset alarm hi/lo (so we don't repeat alarms when very close swings)

	//Check High Alarms;
	if (LidOpened) exit;  //don't check during lidOpen detect.

	if (targetReached == false) { 
								curProbes[pitProbeIndex].AlarmActiveMax = true;   //if pitprob hasn't reached target then don't fire alarms for it (set that alarms are already active)
								curProbes[pitProbeIndex].AlarmActiveMin = true;
								}

	for (int fx = 0; fx < MaxProbes; fx++)  //go through each probe and check for alarm hi
	{
		if (curProbes[fx].curTemp > curProbes[fx].AlarmMaxTemp & curProbes[fx].AlarmMaxTemp > 0)
		{
			if (curProbes[fx].AlarmActiveMax == false)  //new alarm
			{
				//fire alarm high!
				curLink.SendAlarm(fx + 1, "High", curProbes[fx].AlarmMaxTemp, curProbes[fx].curTemp);
				curProbes[fx].AlarmActiveMax = true;								
			}
		}
		else if (curProbes[fx].curTemp < (curProbes[fx].AlarmMaxTemp-alarmBuf)) { curProbes[fx].AlarmActiveMax = false; } //reset alarm if target reached

	}     
	
   //Check Low Alarms;
		for (int fx = 0; fx < MaxProbes; fx++)  //go through each probe and check for alarm lo
		{			
			if (curProbes[fx].curTemp < curProbes[fx].AlarmMinTemp & curProbes[fx].AlarmMinTemp > 0)
			{
				if (curProbes[fx].AlarmActiveMin == false)  //new alarm
				{
					//fire alarm low!
					curLink.SendAlarm(fx + 1, "Low", curProbes[fx].AlarmMinTemp, curProbes[fx].curTemp);
					curProbes[fx].AlarmActiveMin = true;										
				}

			}
			else if (curProbes[fx].curTemp > (curProbes[fx].AlarmMinTemp + alarmBuf)) { curProbes[fx].AlarmActiveMin = false; } //reset alarm if target reached
		}

}

void QControllerClass::LinkSetTemp(int NewTemp)   //when setpoint is changed
{
	//if (NewTemp>=0 && NewTemp<curProbes[pitProbeIndex].maxTemp)
	if (NewTemp >= 0 && NewTemp<MaxSetPoint)
	{
		setpoint = NewTemp;  //set temperature from bluetooth    
		targetReached = false; //reset target for alarms;
		curPID.reset();
		writeConfig(false); //write status eeprom;
	}
}


