/*
globals.h for esp8266 to communicate with QControl

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

#include "globals.h"
#include <myWebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "ThingSpeak.h"
#include "MQTTLink.h"
#include <FS.h>
#include <ArduinoJson.h> 
#include <myAVRFlash.h>
#include <htmlEmbed.h>
//#include <stdarg.h>


GlobalsClass avrGlobal;


#define SoftSerial   //define if using Software serial on 10,11; 
//#define HardSerial   //define if using Software serial on 10,11;  
//#define HardSerialSwap    //swap tx/rx pins...only used if hardserial....
#ifdef SoftSerial
#include <mySoftwareSerial.h>
//SoftwareSerial qCon(4, 5, false, 128);
SoftwareSerial qCon(true);  //mysoftserial must call setup to dynamic setup pins....
#endif

#ifdef HardSerial
HardwareSerial &qCon = Serial;   //we use espSerial to communicate to blynk through cmdMessenger;
#endif





unsigned long ipow10(unsigned power)
{
	const unsigned base = 10;
	unsigned long retval = 1;

	for (int i = 0; i < power; i++) {
		retval *= base;
	}
	return retval;
}


char *float2s(float f, char buf[], unsigned int digits)
{
	// Buffer to build string representation
	int index = 0;       // Position in buf to copy stuff to

						 // For debugging: Uncomment the following line to see what the
						 // function is working on.
						 //Serial.print("In float2s: bytes of f are: ");printBytes(f);

						 // Handle the sign here:
	if (f < 0.0) {
		buf[index++] = '-';
		f = -f;
	}
	// From here on, it's magnitude

	// Handle infinities 
	if (isinf(f)) {
		strcpy(buf + index, "INF");
		return buf;
	}

	// Handle NaNs
	if (isnan(f)) {
		strcpy(buf + index, "NAN");
		return buf;
	}

	//
	// Handle numbers.
	//

	// Six or seven significant decimal digits will have no more than
	// six digits after the decimal point.
	//
	if (digits > 6) {
		digits = 6;
	}

	// "Normalize" into integer part and fractional part
	int exponent = 0;
	if (f >= 10) {
		while (f >= 10) {
			f /= 10;
			++exponent;
		}
	}
	else if ((f > 0) && (f < 1)) {
		while (f < 1) {
			f *= 10;
			--exponent;
		}
	}

	//
	// Now add 0.5 in to the least significant digit that will
	// be printed.

	//float rounder = 0.5/pow(10, digits);
	// Use special power-of-integer function instead of the
	// floating point library function.
	float rounder = 0.5 / ipow10(digits);
	f += rounder;

	//
	// Get the whole number part and the fractional part into integer
	// data variables.
	//
	unsigned intpart = (unsigned)f;
	unsigned long fracpart = (unsigned long)((f - intpart) * 1.0e7);

	//
	// Divide by a power of 10 that zeros out the lower digits
	// so that the "%0.lu" format will give exactly the required number
	// of digits.
	//
	fracpart /= ipow10(6 - digits + 1);

	//
	// Create the format string and use it with sprintf to form
	// the print string.
	//
	char format[16];
	// If digits > 0, print
	//    int part decimal point fraction part and exponent.

	if (digits) {

		sprintf(format, "%%u.%%0%dlue%%+d", digits);
		//
		// To make sure the format is what it is supposed to be, uncomment
		// the following line.
		//Serial.print("format: ");Serial.println(format);
		sprintf(buf + index, format, intpart, fracpart, exponent);
	}
	else { // digits == 0; just print the intpart and the exponent
		sprintf(format, "%%ue%%+d");
		sprintf(buf + index, format, intpart, exponent);
	}

	return buf;
}






void  FlashHM() {  //server request to flash avr file to HM...file exists on spiffs	
	if (!MyWebServer.isAuthorized()) return;

	String fname = "";	
	if (server.hasArg("fname")) { fname=server.arg("fname"); }

	if (fname == "") return server.send(200, "text/html", "Flashing File NOT FOUND");;
	DebugPrintln("FLashing :" + server.arg("fname"));
	MyWebServer.OTAisflashing = true;	
	//delay(200);
#ifdef SoftSerial
	qCon.enableRx(true);
#endif
	qCon.flush();	
//	delay(10);
	//qCon.begin(115200);  //HM speed for flashing with optiboot	
	qCon.begin(Esp8266AVRFlash.AVR_BAUDRATE);
	
	//Esp8266avrFlash.avr_PAGESIZE = 256;
	Esp8266AVRFlash.FlashAVR(&qCon, "/"+fname);  //flashavr HM
	qCon.flush();  	
	server.send(200, "text/html", "Flashing avr....please wait...will auto-reboot...do NOT touch system!!!");
	delay(2000);
	ESP.restart(); //restart ESP after reboot.....
}




void sendHMJsonweb() {


	if (!MyWebServer.isAuthorized()) return;


	String postStr = "{";
	if (avrGlobal.avrPitTemp != "U")  postStr += "\"pittemp\":" + avrGlobal.avrPitTemp + ",";
	if (avrGlobal.avrFood1 != "U") postStr += "\"food1\":" + avrGlobal.avrFood1 + ",";
	if (avrGlobal.avrFood2 != "U") postStr += "\"food2\":" + avrGlobal.avrFood2 + ",";
	if (avrGlobal.avrAmbient != "U") postStr += "\"food3\":" + avrGlobal.avrAmbient + ",";
	if (avrGlobal.avrFanMovAvg != "U") postStr += "\"fanavg\":" + avrGlobal.avrFanMovAvg + ",";
	if (avrGlobal.avrFan != "U") postStr += "\"fancur\":" + avrGlobal.avrFan + ",";
	if (avrGlobal.avrSetPoint != "U") postStr += "\"setpoint\":" + avrGlobal.avrSetPoint + ",";
	if (avrGlobal.avrLidOpenCountdown != "U") postStr += "\"lidopen\":" + avrGlobal.avrLidOpenCountdown + ",";
	if (postStr.charAt(postStr.length() - 1) == ',') postStr.remove(postStr.length() - 1, 1);
	postStr += "}";

	server.send(200, "application/json", postStr);

}


void  setAVRweb() {	    ///hm/set?do=settemp&setpointf=225
	bool isOK = false;
	if (!MyWebServer.isAuthorized()) return;
	if (server.arg("do") == "settemp") {
		if (server.arg("setpointf") != "") {
			int nt=String(server.arg("setpointf")).toInt(); 
			if (nt > 0) {
				avrGlobal.SetTemp(nt);
				server.send(200, "text/html", "Temp Set to : " + String(nt));
				isOK = true;
			}
		}
	}
	if (server.arg("do") == "setalarm") {    //hm/set?do=setalarm&alarms=10,10,20,20,30,30,40,40
		if (server.arg("alarms") != "") {
			String al = server.arg("alarms");
			if (al.length() > 0) {
				avrGlobal.ConfigAlarms(al);
				server.send(200, "text/html", "Alarms Setup Sent");
				isOK = true;
			}
		}
	}
	if (!isOK) { server.send(500, "text/html", "invalid request"); }
}

void  getAVRweb() {	    ///hm/set?do=settemp&setpointf=225
	bool isOK = false;
	if (!MyWebServer.isAuthorized()) return;
/*	if (server.arg("do") == "settemp") {
		if (server.arg("setpointf") != "") {
			int nt = String(server.arg("setpointf")).toInt();
			if (nt > 0) {
				avrGlobal.SetTemp(nt);
				server.send(200, "text/html", "Temp Set to : " + String(nt));
				isOK = true;
			}
		}
	} */
	if (server.arg("do") == "getalarm") {    //hm/set?do=setalarm&alarms=10,10,20,20,30,30,40,40
			String al = avrGlobal.getAlarmsJson();
			if (al.length() > 0) {	
				server.send(200, "application/json", al);
				isOK = true;
			}		
	}
	if (!isOK) { server.send(500, "text/html", "invalid request"); }
}

/*
void createThingSpeakChannel() {   //thingcreate?do=create&key=APIKEY
	bool isOK = false;
	if (!MyWebServer.isAuthorized()) return;
	if (server.arg("do") == "create") {
		if (server.arg("key") != "") {
			String tkey = String(server.arg("key"));
			if (tkey.length() > 0) {
				String resp = ThingSpeak.CreateThingSpeakChannel(tkey);  //returns json of channel
				if (resp.length()>0)
				{
					server.send(200, "application/json", resp);
				} else server.send(500, "text/html", "ERROR...Could Not Create Channel");
				isOK = true;
			}
		}
	}
}

*/

void JsonSaveCallback(String fname)  ///this is the callback funtion when the webserver saves a json file....we check which one and do things....
{
	if (fname == "/heatgeneral.json") avrGlobal.SendHeatGeneralToAVR(fname);   //send avr general to serial port.
	else if (fname == "/heatprobes.json") avrGlobal.SendProbesToAVR(fname);   //send avr probe info to serial port...
	else if (fname == "/cloudgen.json") ThingSpeak.begin();        //reload cloud settings
}


String qMakeMsg(String msgStr)   //qControl serial protocol message to avr,  msgStr needs the message type,  NOT the $$ of * around the string! it adds \n to end
{
	char cs = '\0'; //chksum
	String curChk = "*";
	//calc chksum;

	for (int fx = 0; fx < msgStr.length(); fx++)
	{
		cs ^= msgStr.charAt(fx);
	}
	
	uint8_t val;
	val = byte(cs) / 16;
	val = val < 10 ? val + '0' : val + 'A' - 10;
	curChk += char(val);
	val = byte(cs) % 16;
	val = val < 10 ? val + '0' : val + 'A' - 10;
	curChk += char(val);
	return ("$$"+msgStr+curChk+ '\n');
}





GlobalsClass::GlobalsClass()
{  //init vars
	avrSetPoint = "11";
	avrPitTemp = "11";
	avrFood1 = "U";
	avrFood2 = "U";
	avrAmbient = "U";
	avrFan = "0";
	avrFanMovAvg = "0";
	avrLidOpenCountdown = "0";

	for (int fx = 0; fx < MAX_PROBES; fx++) {
		Probes[fx].AlarmHi=-10;
		Probes[fx].AlarmLo=-10;
		Probes[fx].Name = "Probe " + String(fx + 1);
		Probes[fx].curTemp = 999;
	}


}

void  GlobalsClass::SetTemp(int sndTemp)   //send temperature to HM via serial....
{	
	if (sndTemp>0)
		{
			SendStringToAVR(qMakeMsg("SETP="+String(sndTemp)));
			//qCon.println(String("/set?sp=") + String(sndTemp));
			//qCon.println(String("/set?tt=Remote Temp,Set to ") + String(sndTemp));
			//DebugPrintln(String("Setting Remote Temp ") + String(sndTemp));
			avrSetPoint = String(sndTemp);
		}
	
}




void testgz() {
//	server.sendHeader("Content-Encoding", "gzip");
//	server.send_P(200, "text/html", wifisetup_html_gz, sizeof(wifisetup_html_gz));
//	FileSaveContent_P("/testconfig.html.gz", wifisetup_html_gz, sizeof(wifisetup_html_gz),false);
  
}

void stopAVR()
{
	MyWebServer.isDownloading = true;  //stop all processes from running
	pinMode(byte(avrGlobal.txpin), INPUT);   //this will stop the transfer pin from being high and allow AVR to be programmed
	//must reboot afterwards....	
	server.send(200, "text/html", "AVR link has been stopped...now upload update to AVR.  Must reboot both AVR/Esp once done....");
}





void  GlobalsClass::begin()
{
	//serveron("/probesave", handleProbeSave);	
	//MyWebServer.ServerON("/test", &TestCallback);
    //MyWebServer.CurServer->on("/test", TestCallback);
	server.on("/flashavr", FlashHM);
	server.on("/curinfo", sendHMJsonweb);
	server.on("/setpoint", setAVRweb);
	server.on("/testgz", testgz);
	server.on("/stopavr", stopAVR);
	server.on("/setavr", setAVRweb);
	server.on("/getavr", getAVRweb);
//	server.on("/thingcreate", createThingSpeakChannel); do it in javascript now....
	
	loadSetup(); //load from spiffs


	MyWebServer.jsonSaveHandle = &JsonSaveCallback;  //server on jsonsave file we hook into it to see which one and process....

#ifdef SoftSerial
#include <mySoftwareSerial.h>
	qCon.mySerialSetup(rxpin, txpin, false, buffsize);
	delay(20);
	qCon.begin(baudrate);
	delay(20);
	qCon.enableRx(true);
	delay(20);
#endif

#ifdef HardSerial
		#ifdef HardSerialSwap
			Serial.swap();  //toggle between use of GPIO13/GPIO15 or GPIO3/GPIO(1/2) as RX and TX
		#endif
#endif

	if (WiFi.status() == WL_CONNECTED)
	{
		// ... print IP Address to HM
		qCon.println("/set?tt=WiFi Connected,"+ (String)WiFi.localIP()[0] + "." + (String)WiFi.localIP()[1] + "." + (String)WiFi.localIP()[2] + "." + (String)WiFi.localIP()[3]);
	} else qCon.println("/set?tt=WiFi NOT, Connected!!"); 
	

	lastTimerChk = millis() - (updateInterval * 1000) - 100; //force timer...
}



void  GlobalsClass::SendHeatGeneralToAVR(String fname) {   //sends general info to HM

	String values = "";
	String hmsg;
	File f = SPIFFS.open(fname, "r");	
	if (f) { // we could open the file 
		values = f.readStringUntil('\n');  //read json         
		f.close();
		
		DynamicJsonBuffer jsonBuffer;

		JsonObject& root = jsonBuffer.parseObject(values);  //parse weburl
		if (!root.success())
		{
			DebugPrintln("parseObject() failed");
			return;
		}
		//const char* sensor    = root["sensor"];
		//long        time      = root["time"];
		//double      latitude  = root["data"][0];
		//double      longitude = root["data"][1];           }

		//set PID                 

		SendStringToAVR(qMakeMsg("PROGSTART|ALL"));

		qCon.flush();
		delay(10);

		char jData[300];
		char pidp[15], pidi[15], pidd[15], pidb[15];  //floatstr

		float PID_P, PID_I, PID_D, PID_B;

		PID_P = String(root["pidp"].asString()).toFloat();
		PID_I = String(root["pidi"].asString()).toFloat();
		PID_D = String(root["pidd"].asString()).toFloat();
		PID_B = String(root["pidb"].asString()).toFloat();
			

		sprintf(jData, "p:%s,i:%s,d:%s,b:%s,f:%i", dtostrf(PID_P, 0, 5, pidp), dtostrf(PID_I, 0, 5, pidi), dtostrf(PID_D, 0, 5, pidd), dtostrf(PID_B, 0, 5, pidb), String(root["freq"].asString()).toInt());
		
		SendStringToAVR(qMakeMsg("PID|" + String(jData)));   //send CFGPID		
	
		

		sprintf(jData, "pwm:%i,xp:%i,np:%i,pin:%i,sl:%i",
			String(root["fanpwm"].asString()).toInt(),			
			String(root["maxfan"].asString()).toInt(),
			String(root["minpwm"].asString()).toInt(),
			String(root["gpio"].asString()).toInt(),						
			String(root["srvlow"].asString()).toInt());

		SendStringToAVR(qMakeMsg("CFGF|" + String(jData)));   //send fan   1/2 half

		sprintf(jData, "sh:%i,pm:%i,ms:%i,fd:%i,sp:%i",     //pm is not used?
			String(root["srvhi"].asString()).toInt(),
			String(root["prestr"].asString()).toInt(),
			String(root["maxstr"].asString()).toInt(),
			String(root["fanflr"].asString()).toInt(),
			String(root["srvcl"].asString()).toInt());

		SendStringToAVR(qMakeMsg("CFGF|" + String(jData)));   //send fan  2/2 half
		


		sprintf(jData, "lo:%i,lt:%i,aX:%i,aN:%i",
			String(root["lidoff"].asString()).toInt(),
			String(root["liddur"].asString()).toInt(),						
			String(root["maxset"].asString()).toInt(),
			String(root["prestr"].asString()).toInt());

		SendStringToAVR(qMakeMsg("CFGGEN|" + String(jData)));   //send general
		


		SendStringToAVR(qMakeMsg("SAVE|ALL"));   //SAVE ALL
		

		//qCon.println("debug");
	
	}  //open file success

}



void GlobalsClass::SendProbesToAVR(String fname) {   //sends Probes info to HM
	String values = "";
	String hmsg;
	String alarms;
	File f = SPIFFS.open(fname, "r");
	if (f) { // we could open the file 
		values = f.readStringUntil('\n');  //read json         
		f.close();

		//WRITE CONFIG TO HeaterMeter

		DynamicJsonBuffer jsonBuffer;

		JsonObject& root = jsonBuffer.parseObject(values);  //parse json data
		if (!root.success())
		{
			DebugPrintln("parseObject() failed");
			return;
		}

		SendStringToAVR(qMakeMsg("PROGSTART|ALL"));   //Start Program Mode;

		qCon.flush();
		delay(10);

		char jData[300];

		alarms = "$ALARM,";  //build alarm string ala thingspeak command style

		for (int fx = 0; fx < 4; fx++) {
			//char coa[15], cob[15], coc[15];
			String cP = "p" + String(fx);  //curProbe#			
			
			sprintf(jData, "pn:\"%s\"",    //names
				root[cP+"name"].asString());

			SendStringToAVR(qMakeMsg("P=" + String(fx + 1) + "|" + String(jData)));   //names

			sprintf(jData, "a:%s,b:%s,c:%s",
				root[cP + "a"].asString(),
				root[cP + "b"].asString(),
				root[cP + "c"].asString());   //coefficients

			SendStringToAVR(qMakeMsg("P=" + String(fx + 1) + "|" + String(jData)));  //coefficents

			sprintf(jData, "pt:%i,gp:%i,rs:%li,pf:%i",
				String(root[cP + "conn"].asString()).toInt(),
				String(root[cP + "p"].asString()).toInt(),
				String(root[cP + "r"].asString()).toInt(),
				String(root[cP + "off"].asString()).toInt());

			SendStringToAVR(qMakeMsg("P=" + String(fx + 1) + "|" + String(jData)));  //everything else

			
			alarms += String(root[cP + "all"].asString()) + "," + String(root[cP + "alh"].asString()) +",";  //build alarm setting string;
						

		}  //for each probe

		//send alarm settings like thingspeak command;
		ConfigAlarms(alarms);

		SendStringToAVR(qMakeMsg("SAVE|ALL"));   //SAVE ALL
			
	//	qCon.println("debug");
	}  //open file success
	loadProbes(); //reload probe structs in memory
}


bool GlobalsClass::WaitForAVROK()
{
	unsigned long startTime = millis();
	while (qCon.available()) {
		if (millis() - startTime > 3000) {
			Serial.print("AVRtimeOUT");
			return false; //error, wait 3 seconds for response
		}
				yield();
	}; 
	String res = qCon.readStringUntil('\n');
	Serial.println(res);
	if (res == "AOK") {
		Serial.print("got avr ok ");
		//		yield();
		return true;
	}
	else {
		return false;
	}

}


void GlobalsClass::SendStringToAVR(const String& msgStr)
{	
//	Serial.println(msgStr);
//	for (int fy = 0; fy < msgStr.length(); fy++)
//	{
//		qCon.write(msgStr.charAt(fy));
	//	delay(1);  //prevent serial buffer overrun?
//	}	
	qCon.println(msgStr);
	qCon.flush();
	String res = qCon.readStringUntil('\n');
	Serial.println(res);
	delay(comdelay);
}

void GlobalsClass::handle()
{
	if (qCon.available() > 0) checkSerialMsg();

	if (millis() - lastTimerChk >= updateInterval * 1000)  //POLL AVR FOR INFO;
	{
		lastTimerChk = millis();
		if (MyWebServer.OTAisflashing == false && MyWebServer.isDownloading == false) qCon.println("INFO");  //only send when not in programming eeprom mode.		
	}

	//alarm time checking
	if (ResetTimeCheck > 0) {
		if (millis() - ResetTimeCheck > (ResetAlarmSeconds * 1000)) { ResetAlarms(); }   //check to see if we've past resetalarm;
	}
}

String GlobalsClass::getValue(String data, int index, char separator) {
	int stringData = 0;        //variable to count data part nr 
	String dataPart = "";      //variable to hole the return text

	for (int i = 0; i <= data.length() - 1; i++) {    //Walk through the text one letter at a time

		if (data[i] == separator) {
			//Count the number of times separator character appears in the text
			stringData++;

		}
		else if (stringData == index) {
			//get the text when separator is the rignt one
			dataPart.concat(data[i]);

		}
		else if (stringData>index) {
			//return text and stop if the next separator appears - to save CPU-time
			return dataPart;
			break;

		}

	}
	//return text if this is the last part
	return dataPart;
}

String GlobalsClass::getAlarmsJson()
{
	StaticJsonBuffer<500> jsonBuffer;

	JsonObject& root = jsonBuffer.createObject();
	//root["Info"] = "Alarms";
	//root["time"] = 1351824120;

	JsonArray& pNames = root.createNestedArray("ProbeNames");
	//data.add(48.756080, 6);  // 6 is the number of decimals to print
	//data.add(2.302038, 6);   // if not specified, 2 digits are printed
	for (int fx=0; fx < MAX_PROBES;fx++)	{
		pNames.add(Probes[fx].Name);
	}

	JsonArray& pLo = root.createNestedArray("ProbeAlarmsLo");
	for (int fx=0; fx < MAX_PROBES; fx++)	{
		pLo.add(Probes[fx].AlarmLo);
	}


	JsonArray& pHi = root.createNestedArray("ProbeAlarmsHi");
	for (int fx=0; fx < MAX_PROBES; fx++)	{
		pHi.add(Probes[fx].AlarmHi);
	}

	String str;
	root.printTo(str);
	return str;
}


boolean validatechksum(String msg)
{  //NMEA 0183 format
	
	String tstmsg = msg.substring(1, msg.length() - 3);
	String inCHK = msg.substring(msg.length() - 2, msg.length());
	char cs='\0'; //chksum

	if (msg.charAt(msg.length() - 3) != '*') { return false; }
	
	
	for (int fx = 0; fx < tstmsg.length(); fx++)
	{
		cs ^= tstmsg.charAt(fx);
	}	
	if (cs == (int)strtol(inCHK.c_str(), NULL, 16)) {
		return true;
	}
	else { 
	DebugPrintln("*** Serial MSG CHKSUM FAILED ***");
	MyWebServer.ServerLog("CF:CHKSUM Failed" );
	return false;
	}
}




void  GlobalsClass::checkSerialMsg()
{

	String msgStr = qCon.readStringUntil('\n');
	DebugPrintln("received :" + msgStr);		

	if ((getValue(msgStr, 0) == "$HMSU")) //msg is good updatemsg
	{
		if (validatechksum(msgStr) == false) {
			return;
		}		
		avrSetPoint = getValue(msgStr, 1); if (avrSetPoint == "U") avrSetPoint = "0";
		avrPitTemp = getValue(msgStr, 2);  if (avrPitTemp == "U" | avrPitTemp=="-999") avrPitTemp = "U";
		avrFood1 = getValue(msgStr, 3);    if (avrFood1 == "U" | avrFood1=="-999") avrFood1 = "U";
		avrFood2 = getValue(msgStr, 4);    if (avrFood2 == "U" | avrFood2 == "-999") avrFood2 = "U";
		avrAmbient = getValue(msgStr, 5);  if (avrAmbient == "U" | avrAmbient == "-999") avrAmbient = "U";
		avrFan = getValue(msgStr, 6);    //  if (hmFan == "U") hmFan = "0";
		avrFanMovAvg = getValue(msgStr, 7); //if (hmFanMovAvg == "U") hmFanMovAvg = "0";
		avrLidOpenCountdown = getValue(msgStr, 8);//	if (hmLidOpenCountdown == "U") hmLidOpenCountdown = "0";
	}
	else if ((getValue(msgStr, 0) == "$HMAL")) //Alarm is firing....HM weirdo message
	{
		if (validatechksum(msgStr) == false) return;
		String AlarmInfo;
		bool HasAlarm;

		AlarmInfo = "Pit Alarm! : ";
		HasAlarm = false;
		int msgpos = 1;
		for (int i = 0; i < 4; i++) {
			String AlarmLo;
			String AlarmHi;
			AlarmLo = getValue(msgStr, msgpos);
			if (AlarmLo.charAt(AlarmLo.length() - 1) == 'L')
			{
				AlarmLo.remove(AlarmLo.length() - 1, 1);
				AlarmInfo += "Probe " + String(i + 1) + " Low:  " + AlarmLo + " ! ";
				HasAlarm = true;
			}
			msgpos += 1;
			AlarmHi = getValue(msgStr, msgpos);
			if (AlarmHi.charAt(AlarmHi.length() - 1) == 'H')
			{
				AlarmHi.remove(AlarmHi.length() - 1, 1);
				AlarmInfo += "Probe " + String(i + 1) + " Hi:  " + AlarmHi + " ! ";
				HasAlarm = true;
			}
			msgpos += 1;
		}  //for each probe, check alarms
		//reset alarms
		if (ResetTimeCheck > 0) { HasAlarm = false; }  //if we're already in alarm countdown, ignore alarm....
		if (HasAlarm)	{
			if (ResetAlarmSeconds > 0) { ResetTimeCheck = millis(); }
			else { ResetTimeCheck = 0; }   //reset alarm in x Seconds.
		MQTTLink.SendAlarm(AlarmInfo);
		ThingSpeak.SendAlarm(AlarmInfo);		
		}		
	} else  if ((getValue(msgStr, 0) == "$QCAL")) //Alarm was fired (one time)....QControl  $QCAL,1,Low,CheckTemp,CurTemp     ($QCAL, probe number, Low or High, alarmtemp, curtemp)
	{
		if (validatechksum(msgStr) == false) return;
		String AlarmInfo;
		
		AlarmInfo = "Pit Alarm! : ";
		AlarmInfo += "Probe " + getValue(msgStr, 1) + " : " + getValue(msgStr, 2) + " ! Temp:" + getValue(msgStr, 4) + "(" + getValue(msgStr, 3) + ")";
		MQTTLink.SendAlarm(AlarmInfo);
		ThingSpeak.SendAlarm(AlarmInfo);
	}		
}

void  GlobalsClass::ResetAlarms()
{
	ResetTimeCheck = 0;
	//qCon.println("/set?al=0,0,0,0,0,0,0,0"); delay(comdelay);
	
}

void GlobalsClass::loadSetup()
{
	String values = "";

	File f = SPIFFS.open("/espgen.json", "r");
	if (!f) {
		DebugPrintln("esp config not found");
	}
	else {  //file exists;
		values = f.readStringUntil('\n');  //read json        		
		f.close();

		DynamicJsonBuffer jsonBuffer;

		JsonObject& root = jsonBuffer.parseObject(values);  //parse weburl
		if (!root.success())
		{
			DebugPrintln("parseObject() espfile failed");
			return;
		}
		if (root["ver"].asString() != "") { //verify good json info                                                
			//thingSpeakURL = root["spkurl"].asString();
			rxpin= String(root["rxpin"].asString()).toInt();
			txpin = String(root["txpin"].asString()).toInt();
			resetpin = String(root["rspin"].asString()).toInt();
			baudrate = String(root["baud"].asString()).toInt();
			baudrateFlash = String(root["fbaud"].asString()).toInt();
			Esp8266AVRFlash.AVR_PAGESIZE = String(root["fpage"].asString()).toInt();
			Esp8266AVRFlash.AVR_BAUDRATE = baudrateFlash;
			Esp8266AVRFlash.AVR_RESETPIN = resetpin;
			buffsize = String(root["buff"].asString()).toInt();
			//if (String(root["status"].asString()).toInt() == 1) ThingEnabled = true; else ThingEnabled = false;
			//if (String(root["tbstatus"].asString()).toInt() == 1) TalkBackEnabled = true; else TalkBackEnabled = false;
			DebugPrintln("Esp Settings Loaded...");
		}
	} //file exists;       

	loadProbes();  //on startup load probes
}

void GlobalsClass::loadProbes()
{
	String values = "";		
	File f = SPIFFS.open("/heatprobes.json", "r");
	if (!f) {
		DebugPrintln("probes config not found");
	}
	else {  //file exists;
		values = f.readStringUntil('\n');  //read json        		
		f.close();

		DynamicJsonBuffer jsonBuffer;

		JsonObject& root = jsonBuffer.parseObject(values);  //parse weburl
		if (!root.success())
		{
			DebugPrintln("parseObject() probefile failed");
			return;
		}
		for (int fx = 0; fx < 4; fx++) {
			String cP = "p" + String(fx);  //curProbe#			
			Probes[fx].Name = root[cP + "name"].asString();
			Probes[fx].AlarmLo = String(root[cP + "all"].asString()).toInt();
			Probes[fx].AlarmHi = String(root[cP + "alh"].asString()).toInt();
		}  //for each probe
	} //file exists;        
}

void GlobalsClass::SaveProbes()
{  //used when we change probenames and alarms from GUI
	String values = "";
	String fname = "/heatprobes.json";
	File f = SPIFFS.open(fname, "r");	
	if (!f) {
		DebugPrintln("probes config not found");
	}
	else {  //file exists;
		values = f.readStringUntil('\n');  //read json        		
		f.close();

		DynamicJsonBuffer jsonBuffer;

		JsonObject& root = jsonBuffer.parseObject(values);  //parse weburl
		if (!root.success())
		{
			DebugPrintln("parseObject() probefile failed");
			return;
		}
		for (int fx = 0; fx < 4; fx++) {
			String cP = "p" + String(fx);  //curProbe#			
			root[cP + "name"] = Probes[fx].Name;
			root[cP + "all"] = Probes[fx].AlarmLo;
			root[cP + "alh"] = Probes[fx].AlarmHi;
		}  //for each probe

		//create file and save; 
		String newvalues;
		root.printTo(newvalues);
		File file = SPIFFS.open(fname, "w");
		if (file) {
			file.println(newvalues);  //save json data
			file.close();
		}
	} //file exists;        
}






void  GlobalsClass::ConfigAlarms(String msgStr)
{   //format is $ALARM,10,20,30,40,50,60,70,80   (lo/hi pairs);  send to comport;
	msgStr.replace("$ALARM,", "");  //remove the alarm command and send rest to HM
	SendStringToAVR(qMakeMsg("SA|" + msgStr));   //send ProbeInfo to avr	
	int fy = 0;
	for (int fx = 0; fx < MAX_PROBES; fx++) {
		Probes[fx].AlarmLo = String(getValue(msgStr, fy)).toInt(); fy++;
		Probes[fx].AlarmHi = String(getValue(msgStr, fy)).toInt(); fy++;
	}
	SaveProbes();//save to file
//	qCon.println("/set?al="+msgStr); delay(comdelay);
	DebugPrintln("setting new alarms " + msgStr);
//	qCon.println("/set?tt=Web Alarms,Updated.."); delay(comdelay);
}

