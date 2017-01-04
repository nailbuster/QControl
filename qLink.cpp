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

#include "qLink.h"
#include "qGlobals.h"








#ifdef SoftSerial
SoftwareSerial Serial(A3, A4);   //we use Serial to communicate to blynk through cmdMessenger;
#endif

#ifdef HardSerial
HardwareSerial &Serial = Serial;   //we use Serial to communicate to blynk through cmdMessenger;
#endif




QLinkClass::QLinkClass()
{
}

void QLinkClass::begin()
{
#ifdef SoftSerial 
	Serial.begin(38400); delay(19); Serial.println(F("using SoftSerial"));
#endif

#ifdef HardSerial 
	Serial.println(F("using hardware Serial"));
#endif
}

void QLinkClass::run()
{
	if (Serial.available() >0) 
	{
		//char sr = char(Serial.read());		
		//if (sr != '\n') msgStr += sr;  else checkSerialMsg();
		msgStr = Serial.readStringUntil('\n');		
		if (msgStr.indexOf("INFO") == 0) {   //check if its a get "INFO" message
			if (inProgMode == false) updateInfoEsp();  //only send when not in programming eeprom mode.
			msgStr == "";
		} else checkSerialMsg();
	}
		
		
	//	checkSerialMsg();  //check for messages from esp....

/*	if (millis() - lastTimerChk >= updateInterval * 1000)
	{
		lastTimerChk = millis();
		if (inProgMode==false) updateInfoEsp();  //only send when not in programming eeprom mode.
	}
*/
}


//My Message structure is $$AB|....|...|...|...|...|...*CHECKSUM\n     after $$ is msg type (ABC=4),  everything else is json-ish....sp=12..| is terminator  checksum is 2 char hex digits cs ^= tstmsg.charAt(fx) or all chars after $$ and before *

void QLinkClass::checkSerialMsg()   //Process Serial MSG from device.....
{	

	bool msgGood = false;
	char charSep = '|';
	

	//String msgStr = msgBuffer;
	//msgBuffer = "";
	
	//Serial.println(msgStr);

	//debug
	if (msgStr.indexOf("debug") == 0) {		//this will send all config data to serial port for debug....
		char json[300];
		curQ.AsJson(json);Serial.println(json);
		curFan.AsJson(json); Serial.println(json);
		curPID.AsJson(json); Serial.println(json);
		curProbes[0].AsJson(json); Serial.println(json);
		curProbes[1].AsJson(json); Serial.println(json);
		curProbes[2].AsJson(json); Serial.println(json);
		curProbes[3].AsJson(json); Serial.println(json);
	}
	if (msgStr.indexOf("$$") == 0) {   //valid start header

		//validate CHKSUM and strip header/chksum from msg

		msgStr.remove(0, 2);  //strip header;	

		String CHKSUM = msgStr.substring(msgStr.length() - 2, msgStr.length());   //grab last two chars as chksum

		if (msgStr.charAt(msgStr.length() - 3) != '*') { msgStr = ""; Serial.println("msg no good");  inProgMode = false;  return; }  //exit if invalid chksum struct at end... end should be *[2 char chksum]

		msgStr.remove(msgStr.length() - 3, 3);  //strip chksum from msg 
		
		char cs = '\0'; //chksum

		//calc chksum;

		for (int fx = 0; fx < msgStr.length(); fx++)
		{
			cs ^= msgStr.charAt(fx);
		}

		//debug 
		//uint8_t val;
		//val = byte(cs) / 16;
		//val = val < 10 ? val + '0' : val + 'A' - 10;
		//Serial.write(val); 
		//val = byte(cs) % 16;
		//val = val < 10 ? val + '0' : val + 'A' - 10;
		//Serial.write(val);		

		if (cs != (int)strtol(CHKSUM.c_str(), NULL, 16)) {        //valid chksum?
			msgStr = "";
			Serial.println(F("***CHKSUM FAIL ***"));
			Serial.println(msgStr);			
			inProgMode = false;
			return;
		}

		//everything good....lets use msg!!
		msgGood = true;

		String curValue, curField, curPart;

		curPart = String(getValue(msgStr, 0, charSep));  //grab first Part as message type;

		curField = String(getValue(curPart, 0, '='));   //msg type can be in format "p=1", or just "F"
		curValue = String(getValue(curPart, 1, '='));

		if (curField == "CFGF")  //fan config msg
		{   //with cfg as json msg we strip first part and send to object json values;			
			msgStr.remove(0, curPart.length() + 1);  //remove msg type from msg data
			curFan.SetValuesJson(msgStr);		
		}
		else if (curField == "PROGSTART")  //save all settings
		{
			Serial.println(F("PROG START"));
			inProgMode = true;
		}
		else if (curField == "SAVE")  //save all settings
		{
			Serial.println(F("saving settings"));
			curQ.writeConfig(true);
			msgStr = "";
			Serial.println("AOK");
			inProgMode = false;
			return;
		}
		else if (curField == "SETP")  // SETP=500;  //sets point in f
		{			
			curQ.LinkSetTemp(curValue.toInt());
		}
		else if (curField == "PID")  //PID configuration
		{
			msgStr.remove(0, curPart.length() + 1);  //remove msg type from msg data
			curPID.SetValuesJson(msgStr);			
		}
		else if (curField == "CFGGEN")  //GENERAL Controller Config
		{
			msgStr.remove(0, curPart.length() + 1);  //remove msg type from msg data
			curQ.SetValuesJson(msgStr);			
		}
		else if (curField == "SA")   // SETALRM|1L,1H,2L,2H,3L,3H,4L,4H*xx
		{									
			curProbes[0].AlarmMinTemp = String(getValue(msgStr,1,',')).toInt();
			curProbes[0].AlarmMaxTemp = String(getValue(msgStr,2,',')).toInt();
			curProbes[1].AlarmMinTemp = String(getValue(msgStr,3,',')).toInt();
			curProbes[1].AlarmMaxTemp = String(getValue(msgStr,4,',')).toInt();
			curProbes[2].AlarmMinTemp = String(getValue(msgStr,5,',')).toInt();
			curProbes[2].AlarmMaxTemp = String(getValue(msgStr,6,',')).toInt();
			curProbes[3].AlarmMinTemp = String(getValue(msgStr,7,',')).toInt();
			curProbes[3].AlarmMaxTemp = String(getValue(msgStr,8,',')).toInt();
		}
		else if (curField == "P")  //CFGP=y|{.....}*[chksum]    y is probe #  1=first pit....4=food
		{
			msgStr.remove(0, curPart.length() + 1);  //remove msg type from msg data
			//get probe index from 			
			byte curp = curValue.toInt();
			curp=constrain(curp, 1, MaxProbes);						
			curProbes[curp - 1].SetValuesJson(msgStr);						
		}
	

	}
	msgStr = "";
	if (msgGood) { Serial.println("AOK");}
}

void hexwrite(uint8_t val) 
{
	val = val < 10 ? val + '0' : val + 'A' - 10;
	Serial.write(val);
};

void QLinkClass::updateInfoEsp()
{
	char eData[200];
	//Serial.flush();
	Serial.flush();
	//Serial.println(F("Sending Esp Values"));
	fBuf(eData, 200, F("$HMSU,%i,%i,%i,%i,%i,%i,%i,%i,"),curQ.setpoint, curQ.pit_temp, curProbes[1].curTemp, curProbes[2].curTemp, curProbes[3].curTemp, curFan.duty100(0), curFan.duty100(1), curQ.LidOpenCountdown);	

	char cs = '\0'; //chksum
	for (int fx = 1; fx < String(eData).length(); fx++)	
	{
		cs ^= eData[fx];
	}
	//Serial.println(byte(cs));
	Serial.print(String(eData)); Serial.print("*"); hexwrite(byte(cs) / 16); hexwrite(byte(cs) % 16); Serial.print('\n');  //send to esp with checksum
}



void QLinkClass::SendAlarm(int ProbNum, String AlarmType, int AlarmCheckTemp, int CurTemp)
//QControl  $QCAL, 1, Low, CheckTemp, CurTemp($QCAL, probe number, Low or High, alarmtemp, curtemp)
{
	char eData[200];
	Serial.flush();
	fBuf(eData, 200, F("$QCAL,%i,%s,%i,%i,"), ProbNum,AlarmType.c_str(),AlarmCheckTemp,CurTemp);
	char cs = '\0'; //chksum
	for (int fx = 1; fx < String(eData).length(); fx++)
	{
		cs ^= eData[fx];
	}
	//Serial.println(byte(cs));
	Serial.print(String(eData)); Serial.print("*"); hexwrite(byte(cs) / 16); hexwrite(byte(cs) % 16); Serial.print('\n');  //send to esp with checksum
}




