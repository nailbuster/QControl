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
#include <myWebServerAsync.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "ThingSpeak.h"
#include "MQTTLink.h"
#include <FS.h>
#include <ArduinoJson.h> 
#include <myAVRFlash.h>
#include <htmlEmbedBig.h>
#include <TimeLib.h>
#include "LogDataClass.h"
#include "qcExtras.h"
//#include <stdarg.h>


#ifdef HEATERMETER
HMGlobalClass avrGlobal;
#else
GlobalsClass avrGlobal;
#endif


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







void FlashHMAsync() {
	WebServer.OTAisflashing = true;
	//delay(200);
#ifdef SoftSerial
	qCon.enableRx(true);
#endif
	qCon.flush();
	//	delay(10);
	//qCon.begin(115200);  //HM speed for flashing with optiboot	
	qCon.begin(Esp8266AVRFlash.AVR_BAUDRATE);

	//Esp8266avrFlash.avr_PAGESIZE = 256;
	Esp8266AVRFlash.FlashAVR(&qCon, "/" + avrGlobal.FlashHMFileAsync);  //flashavr HM
	qCon.flush();
	delay(2000);
	ESP.restart(); //restart ESP after reboot.....
}





void  FlashHM(AsyncWebServerRequest *request) {  //server request to flash avr file to HM...file exists on spiffs	
	if (!WebServer.isAuthorized(request)) return;

	String fname = "";	
	if (request->hasParam("fname")) { fname=request->arg("fname"); }

	if (fname == "") return request->send(200, "text/html", "Flashing File NOT FOUND");;
	DebugPrintln("FLashing :" + request->arg("fname"));
	avrGlobal.FlashHMFileAsync = fname;
	request->send(200, "text/html", "Flashing avr....please wait...will auto-reboot...do NOT touch system!!!");
	
	
}




void sendCurInfo(AsyncWebServerRequest *request) {


	if (!WebServer.isAuthorized(request)) return;

	
	DynamicJsonBuffer jsonBuffer;

	JsonObject& root = jsonBuffer.parseObject("{}");  //parse weburl

	root["pittemp"] = avrGlobal.curData.PitTemp;
	root["food1"] = avrGlobal.curData.Food1;
	root["food2"] = avrGlobal.curData.Food2;
	root["food3"] = avrGlobal.curData.Food3;
	root["fancur"] = avrGlobal.curData.Fan;
	root["fanavg"] = avrGlobal.curData.FanAvg;
	root["setpoint"] = avrGlobal.curData.SetPoint;
	root["lidopen"] = avrGlobal.curData.LidCountDown;	
	root["status"] = CookStatus.status;
	for (int fx = 0; fx < 4; fx++) {
		root["STimer" + String(fx)] = CookStatus.StartTime[fx];
		root["ETimer" + String(fx)] = CookStatus.EndTime[fx];
		root["PN" + String(fx)] = avrGlobal.Probes[fx].Name;
	}  //for each probe


	String newvalues;

	root.printTo(newvalues);

	AsyncWebServerResponse *response = request->beginResponse(200, "application/json", newvalues);
	response->addHeader("Access-Control-Allow-Origin", "*");
	request->send(response);	

}


void sendHMJsonweb(AsyncWebServerRequest *request) {

	String postStr = "{";
	postStr += "\"time\":" + String(now()) + ",";
	postStr += "\"set\":" + avrGlobal.avrSetPoint + ",";
	postStr += "\"lid\":" + avrGlobal.avrLidOpenCountdown + ",";

	postStr += "\"fan\":{";
	postStr += "\"c\":" + avrGlobal.avrFan + ",";
	postStr += "\"a\":" + avrGlobal.avrFanMovAvg; //+ ",";
												//postStr += "\"f\":0"; //TODO: Add "f" and uncomment comma in line above
	postStr += "},";


	//TODO: Add "adc"
	//...

	postStr += "\"temps\":[";
	for (int i = 0; i<4; i++)
	{
		postStr += "{";
		postStr += "\"n\":\"" + avrGlobal.Probes[i].Name + "\",";

		String t = (avrGlobal.Probes[i].curTemp == 0) ? "null" : String(avrGlobal.Probes[i].curTemp);
		postStr += "\"c\":" + t + ",";

		//postStr += "\"dph\":0,"; //TODO: Calculate and update dph (degrees per hour)
		postStr += "\"a\":{\"l\":" + String(avrGlobal.Probes[i].AlarmLo) + ",\"h\":" + String(avrGlobal.Probes[i].AlarmHi) + ",\"r\":" + avrGlobal.AlarmRinging[i] + "}";

		if (i <= 2) postStr += "},";
	}
	postStr += "}]";

	postStr += "}";

	request->send(200, "application/json", postStr);
}


void  setAVRweb(AsyncWebServerRequest *request) {	    ///hm/set?do=settemp&setpointf=225
	bool isOK = false;
	if (!WebServer.isAuthorized(request)) return;
	//server.sendHeader("Access-Control-Allow-Origin", "*");
	if (request->arg("do") == "settemp") {
		if (request->arg("setpointf") != "") {
			int nt=String(request->arg("setpointf")).toInt();
			if (nt > 0) {
				avrGlobal.SetTemp(nt);
				request->send(200, "text/html", "Temp Set to : " + String(nt));
				isOK = true;
			}
		}
	}
	if (request->arg("do") == "setalarm") {    //hm/set?do=setalarm&alarms=10,10,20,20,30,30,40,40
		if (request->arg("alarms") != "") {
			String al = request->arg("alarms");
			if (al.length() > 0) {
				avrGlobal.ConfigAlarms(al,true);   //async
				request->send(200, "text/html", "Alarms Setup Sent");
				isOK = true;
			}
		}
	}
	if (!isOK) { request->send(500, "text/html", "invalid request"); }
}

void  getAVRweb(AsyncWebServerRequest *request) {	    ///hm/set?do=settemp&setpointf=225
	bool isOK = false;
	if (!WebServer.isAuthorized(request)) return;
	//server.sendHeader("Access-Control-Allow-Origin", "*");
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
	if (request->arg("do") == "getalarm") {    //hm/set?do=setalarm&alarms=10,10,20,20,30,30,40,40
			String al = avrGlobal.getAlarmsJson();
			if (al.length() > 0) {	
				request->send(200, "application/json", al);
				isOK = true;
			}		
	}
	if (!isOK) { request->send(500, "text/html", "invalid request"); }
}

/*
void createThingSpeakChannel() {   //thingcreate?do=create&key=APIKEY
	bool isOK = false;
	if (!WebServer.isAuthorized()) return;
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


String qMakeMsg(String msgStr)   //qControl serial protocol message to avr,  msgStr needs the message type,  NOT the $$ or * around the string! it adds \n to end
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
		Probes[fx].curTemp = -999;
	}


}

void  GlobalsClass::SetTemp(int sndTemp)   //send temperature to HM via serial....will do it async/background
{	
	if (sndTemp>0)
		{
			SendStringToAVR(qMakeMsg("SETP="+String(sndTemp)));
			avrSetPoint = String(sndTemp);
		}
	
}


void sendCurGraphData(AsyncWebServerRequest *request) {	
	request->send(200, "text/plain", AVRLogData.GetCurGraphData());
}

void testgz(AsyncWebServerRequest *request) {
//	server.sendHeader("Content-Encoding", "gzip");
//	server.send_P(200, "text/html", wifisetup_html_gz, sizeof(wifisetup_html_gz));
//	FileSaveContent_P("/testconfig.html.gz", wifisetup_html_gz, sizeof(wifisetup_html_gz),false);
	//server.sendHeader("Location", "http://code.jquery.com/jquery-2.2.4.min.js", true);
	//server.send(302, "application/javascript", "");
	//request->send(200, "text/plain", server.client().remoteIP().toString());
}

void stopAVR(AsyncWebServerRequest *request)
{
	WebServer.isDownloading = true;  //stop all processes from running
	pinMode(byte(avrGlobal.txpin), INPUT);   //this will stop the transfer pin from being high and allow AVR to be programmed
	//must reboot afterwards....	
	request->send(200, "text/html", "AVR link has been stopped...now upload update to AVR.  Must reboot both AVR/Esp once done....");
}


void getAVRDebug(AsyncWebServerRequest *request) {  //get debug info from avr on serial

	qCon.println("debug");
	request->send(200, "text/html", "Debug info on serial...");
}


void handleCook(AsyncWebServerRequest *request) {//                        /curcook?do=timerstart&timer=3
	bool isOK = false;
	if (!WebServer.isAuthorized(request)) return;
	//server.sendHeader("Access-Control-Allow-Origin", "*");

	if (request->arg("do") == "cookstart") {
		CookStatus.StartCook();
		if (request->arg("setpointf") != "") {  //if setting point when starting....
			int nt = String(request->arg("setpointf")).toInt();
			if (nt > 0) avrGlobal.SetTemp(nt);
		}
		request->send(200, "text/plain", "OK");
		isOK = true;
	}
	else if (request->arg("do") == "cookend") {
		CookStatus.EndCook();
		avrGlobal.SetTemp(1);  //when we stop cook we set pit temp to 1?
		request->send(200, "text/plain", "OK END");
		isOK = true;
	}
	else if (request->arg("do") == "timerstart") {  //timer=(1-3)
		if (request->arg("timer") != "") {
			int nt = String(request->arg("timer")).toInt();
			if (nt > 0 && nt < 4) {
				CookStatus.StartTimer(nt);
				request->send(200, "text/plain", "OK");
				isOK = true;
			}
		}		
	}
	else if (request->arg("do") == "timerend") {  //timer=(1-3)
		if (request->arg("timer") != "") {
			int nt = String(request->arg("timer")).toInt();
			if (nt > 0 && nt < 4) {
				CookStatus.StopTimer(nt);
				request->send(200, "text/plain", "OK");
				isOK = true;
			}
		}
	}
	else if (request->arg("do") == "info") {  //debug					
		request->send(200, "text/plain", CookStatus.AsJSON() );
					isOK = true;
				}
	if (!isOK) { request->send(500, "text/html", "invalid request"); }
}




void  GlobalsClass::begin()
{
	//serveron("/probesave", handleProbeSave);	
	//WebServer.ServerON("/test", &TestCallback);
    //WebServer.CurServer->on("/test", TestCallback);
	server.on("/flashavr", FlashHM);
	server.on("/curinfo", sendCurInfo);
	server.on("/setpoint", setAVRweb);
	server.on("/testgz", testgz);
	server.on("/stopavr", stopAVR);
	server.on("/setavr", setAVRweb);
	server.on("/getavr", getAVRweb);
	server.on("/debug", getAVRDebug);
	server.on("/graphcurdata", sendCurGraphData);
	server.on("/curcook", handleCook);
	
	//HM compat
	server.on("/luci/lm/hmstatus", sendHMJsonweb);
	server.on("/luci/admin/lm", [](AsyncWebServerRequest *request) {request->send(301, "text/html", ""); });
	server.on("/luci/admin/lm/hist", [](AsyncWebServerRequest *request) {request->send(200, "text/html", "1405344600, 65, 94.043333333333, nan, 44.475555555556, 82.325555555556, 0\n"); });



	loadSetup(); //load from spiffs

	
	WebServer.jsonSaveHandle = &JsonSaveCallback;  //server on jsonsave file we hook into it to see which one and process....

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
	CookStatus.begin(); 
	AVRLogData.begin();  //ensure log file is not too big for local logging
}




void SendStringToAVRASYNC(const String& msgStr) {
	qCon.println(msgStr);
	qCon.flush();
	String res = qCon.readStringUntil('\n');
	Serial.println(res);
	delay(comdelay);
}





void GlobalsClass::SendStringToAVR(const String& msgStr)  //async, we need to set string for loop sending in main thread
{
	AVRStringAsync = msgStr;
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

		SendStringToAVRASYNC(qMakeMsg("PROGSTART|ALL"));

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
		
		SendStringToAVRASYNC(qMakeMsg("PID|" + String(jData)));   //send CFGPID		
	
		

		sprintf(jData, "pwm:%i,xp:%i,np:%i,pin:%i,sl:%i",
			String(root["fanpwm"].asString()).toInt(),			
			String(root["maxfan"].asString()).toInt(),
			String(root["minpwm"].asString()).toInt(),
			String(root["gpio"].asString()).toInt(),						
			String(root["srvlow"].asString()).toInt());

		SendStringToAVRASYNC(qMakeMsg("CFGF|" + String(jData)));   //send fan   1/2 half

		sprintf(jData, "sh:%i,pm:%i,ms:%i,fd:%i,sp:%i",     //pm is not used?
			String(root["srvhi"].asString()).toInt(),
			String(root["prestr"].asString()).toInt(),
			String(root["maxstr"].asString()).toInt(),
			String(root["fanflr"].asString()).toInt(),
			String(root["srvcl"].asString()).toInt());

		SendStringToAVRASYNC(qMakeMsg("CFGF|" + String(jData)));   //send fan  2/2 half
		


		sprintf(jData, "lo:%i,lt:%i,aX:%i,aN:%i",
			String(root["lidoff"].asString()).toInt(),
			String(root["liddur"].asString()).toInt(),						
			String(root["maxset"].asString()).toInt(),
			String(root["prestr"].asString()).toInt());

		SendStringToAVRASYNC(qMakeMsg("CFGGEN|" + String(jData)));   //send general
		


		SendStringToAVRASYNC(qMakeMsg("SAVE|ALL"));   //SAVE ALL
		

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

		SendStringToAVRASYNC(qMakeMsg("PROGSTART|ALL"));   //Start Program Mode;

		qCon.flush();
		delay(10);

		char jData[300];

		alarms = "$ALARM,";  //build alarm string ala thingspeak command style

		for (int fx = 0; fx < 4; fx++) {
			//char coa[15], cob[15], coc[15];
			String cP = "p" + String(fx);  //curProbe#			
			
			sprintf(jData, "pn:\"%s\"",    //names
				root[cP+"name"].asString());
			Probes[fx].Name = root[cP + "name"].asString();

			SendStringToAVRASYNC(qMakeMsg("P=" + String(fx + 1) + "|" + String(jData)));   //names

			sprintf(jData, "a:%s,b:%s,c:%s",
				root[cP + "a"].asString(),
				root[cP + "b"].asString(),
				root[cP + "c"].asString());   //coefficients

			SendStringToAVRASYNC(qMakeMsg("P=" + String(fx + 1) + "|" + String(jData)));  //coefficents

			sprintf(jData, "pt:%i,gp:%i,rs:%li,pf:%i",
				String(root[cP + "conn"].asString()).toInt(),
				String(root[cP + "p"].asString()).toInt(),
				String(root[cP + "r"].asString()).toInt(),
				String(root[cP + "off"].asString()).toInt());

			SendStringToAVRASYNC(qMakeMsg("P=" + String(fx + 1) + "|" + String(jData)));  //everything else

			
			alarms += String(root[cP + "all"].asString()) + "," + String(root[cP + "alh"].asString()) +",";  //build alarm setting string;
						

		}  //for each probe

		//send alarm settings like thingspeak command;
		ConfigAlarms(alarms);

		SendStringToAVRASYNC(qMakeMsg("SAVE|ALL"));   //SAVE ALL
			
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



void GlobalsClass::handle()
{	
	if (qCon.available() > 0) checkSerialMsg();


#ifndef HEATERMETER     //qcontrol we poll for info instead of auto-receiving

	if (millis() - lastTimerChk >= updateInterval * 1000)  //POLL AVR FOR INFO;
	{
		lastTimerChk = millis();
		if (WebServer.OTAisflashing == false && WebServer.isDownloading == false) qCon.println("INFO");  //only send when not in programming eeprom mode.		
	}

#endif // !HEATERMETER

	
	//alarm time checking
	if (ResetTimeCheck > 0) {
		if (millis() - ResetTimeCheck > (ResetAlarmSeconds * 1000)) { ResetAlarms(); }   //check to see if we've past resetalarm;
	}

	if (AVRStringAsync != "") {                     //async,  can't send to avr within web request so we have to do in main handle...
		SendStringToAVRASYNC(AVRStringAsync);
		AVRStringAsync = "";
	}

	if (FlashHMFileAsync != "") {
		FlashHMAsync();
		FlashHMFileAsync = "";
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
	WebServer.ServerLog("CF:CHKSUM Failed" );
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
		curData.SetPoint = avrSetPoint.toInt();
		curData.PitTemp =  avrPitTemp.toInt();
		curData.Food1 = avrFood1.toInt();
		curData.Food2 = avrFood2.toInt();
		curData.Food3 = avrAmbient.toInt();
		curData.Fan = avrFan.toInt();
		curData.FanAvg = avrFanMovAvg.toInt();
		curData.LidCountDown = avrLidOpenCountdown.toInt();
		curData.readTime = now();
		Probes[0].curTemp = curData.PitTemp;
		Probes[1].curTemp = curData.Food1;
		Probes[2].curTemp = curData.Food2;
		Probes[3].curTemp = curData.Food3;
		if (CookStatus.status==1) AVRLogData.LogData(curData);  //log locally when cooking is active;
	}
	else if ((getValue(msgStr, 0) == "$HMAL")) //Alarm is firing....
	{
		if (validatechksum(msgStr) == false) return;
		String AlarmInfo;
		bool HasAlarm;

		AlarmInfo = "Pit Alarm! : ";
		HasAlarm = false;
		int msgpos = 1;
		for (int i = 0; i < 4; i++) {
			bool ringing = false;
			String AlarmLo;
			String AlarmHi;
			AlarmLo = getValue(msgStr, msgpos);
			if (AlarmLo.charAt(AlarmLo.length() - 1) == 'L')
			{
				AlarmLo.remove(AlarmLo.length() - 1, 1);
				AlarmInfo += "Probe " + String(i + 1) + " Low:  " + AlarmLo + " ! ";
				HasAlarm = true;
				AlarmRinging[i] = "\"L\"";
				ringing = true;
			}
			msgpos += 1;
			AlarmHi = getValue(msgStr, msgpos);
			if (AlarmHi.charAt(AlarmHi.length() - 1) == 'H')
			{
				AlarmHi.remove(AlarmHi.length() - 1, 1);
				AlarmInfo += "Probe " + String(i + 1) + " Hi:  " + AlarmHi + " ! ";
				HasAlarm = true;
				AlarmRinging[i] = "\"H\"";
				ringing = true;
			}

			if (ringing == false)
			{
				AlarmRinging[i] = "null";
			}

			msgpos += 1;
		}  //for each probe, check alarms
		   //reset alarms
		if (ResetTimeCheck > 0) { HasAlarm = false; }  //if we're already in alarm countdown, ignore alarm....
		if (HasAlarm) {
			if (ResetAlarmSeconds > 0) { ResetTimeCheck = millis(); }
			else { ResetTimeCheck = 0; }   //reset alarm in x Seconds.
			MQTTLink.SendAlarm(AlarmInfo);
			ThingSpeak.SendAlarm(AlarmInfo);
		}
	} else  if ((getValue(msgStr, 0) == "$QCAL")) //Alarm was fired (one time)....QControl  $QCAL,PitName1,Low,CheckTemp,CurTemp     ($QCAL, probe Name, Low or High, alarmtemp, curtemp)
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
			AlarmRinging[fx] = "null";
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
		for (int fx = 0; fx < MAX_PROBES; fx++) {
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






void  GlobalsClass::ConfigAlarms(String msgStr, bool Async)   //async will add to buffer 
{   //format is $ALARM,10,20,30,40,50,60,70,80   (lo/hi pairs);  send to comport;
	msgStr.replace("$ALARM,", "");  //remove the alarm command and send rest to HM
	if (Async) 
		SendStringToAVR(qMakeMsg("SA|" + msgStr));   //send ProbeInfo to avr (will use async)	
	 else
		SendStringToAVRASYNC(qMakeMsg("SA|" + msgStr));   //send ProbeInfo directly to avr	
	
	int fy = 0;
	for (int fx = 0; fx < MAX_PROBES; fx++) {
		Probes[fx].AlarmLo = String(getValue(msgStr, fy)).toInt(); fy++;
		Probes[fx].AlarmHi = String(getValue(msgStr, fy)).toInt(); fy++;
	}
	SaveProbes();//save to file
	DebugPrintln("setting new alarms " + msgStr);
}

void HMGlobalClass::SetTemp(int sndTemp)
{
	if (sndTemp>0)
	{
		qCon.println(String("/set?sp=") + String(sndTemp));
		qCon.println(String("/set?tt=Remote Temp,Set to ") + String(sndTemp));
		DebugPrintln(String("Setting Remote Temp ") + String(sndTemp));
		avrSetPoint = String(sndTemp);
	}
}

void HMGlobalClass::SendHeatGeneralToAVR(String fname)
{

	String values = "";
	String hmsg;
	File f = SPIFFS.open(fname, "r");
	if (f) { // we could open the file 
		values = f.readStringUntil('\n');  //read json         
		f.close();

		//WRITE CONFIG TO HeaterMeter
		//fBuf(sbuf,"/set?sp=%iF",299);  //format command;
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

		qCon.println(String("/set?pidb=") + root["pidb"].asString()); delay(comdelay);
		qCon.println(String("/set?pidp=") + root["pidp"].asString()); delay(comdelay);
		qCon.println(String("/set?pidi=") + root["pidi"].asString()); delay(comdelay);
		qCon.println(String("/set?pidd=") + root["pidd"].asString()); delay(comdelay);

		//Set Fan info /set?fn=FL,FH,SL,SH,Flags,MSS,FAF,SAC 
		hmsg = String("/set?fn=") + root["minfan"].asString() + "," + root["maxfan"].asString() + "," + root["srvlow"].asString() + "," + root["srvhi"].asString() + "," + root["fanflg"].asString() + "," +
			root["maxstr"].asString() + "," + root["fanflr"].asString() + "," + root["srvcl"].asString();

		qCon.println(hmsg); delay(comdelay);
		DebugPrintln(hmsg);
		//Set Display props       
		hmsg = String("/set?lb=") + root["blrange"].asString() + "," + root["hsmode"].asString() + "," + root["ledcfg"].asString();
		qCon.println(hmsg); delay(comdelay);
		DebugPrintln(hmsg);
		//Set Lid props    
		hmsg = String("/set?ld=") + root["lidoff"].asString() + "," + root["liddur"].asString();
		qCon.println(hmsg); delay(comdelay);
		DebugPrintln(hmsg);
		qCon.println("/set?tt=Web Settings,Updated!!"); delay(comdelay);
		qCon.println("/save?"); delay(comdelay);

	}  //open file success


}

void HMGlobalClass::SendProbesToAVR(String fname)
{   	String values = "";
	String hmsg;
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
		//const char* sensor    = root["sensor"];
		//long        time      = root["time"];
		//double      latitude  = root["data"][0];
		//double      longitude = root["data"][1];           }

		//NOTE: Need to update global probe names here (ReadProbesJSON function only called on start-up)
		for (int fx = 0; fx < 4; fx++) {
			String cP = "p" + String(fx);  //curProbe#			
			Probes[fx].Name = root[cP + "name"].asString();
			Probes[fx].AlarmLo = String(root[cP + "all"].asString()).toInt();
			Probes[fx].AlarmHi = String(root[cP + "alh"].asString()).toInt();
		}  //for each prob

		qCon.println(String("/set?pn0=") + Probes[0].Name); delay(comdelay);
		qCon.println(String("/set?pn1=") + Probes[1].Name); delay(comdelay);
		qCon.println(String("/set?pn2=") + Probes[2].Name); delay(comdelay);
		qCon.println(String("/set?pn3=") + Probes[3].Name); delay(comdelay);

		//Set offsets
		hmsg = String("/set?po=") + root["p0off"].asString() + "," + root["p1off"].asString() + "," + root["p2off"].asString() + "," + root["p3off"].asString();
		qCon.println(hmsg); delay(comdelay);
		DebugPrintln(hmsg);

		//Set Probe coeff.
		hmsg = String("/set?pc0=") + root["p0a"].asString() + "," + root["p0b"].asString() + "," + root["p0c"].asString() + "," + root["p0r"].asString() + "," + root["p0trm"].asString();
		qCon.println(hmsg); delay(comdelay);
		DebugPrintln(hmsg);
		hmsg = String("/set?pc1=") + root["p1a"].asString() + "," + root["p1b"].asString() + "," + root["p1c"].asString() + "," + root["p1r"].asString() + "," + root["p1trm"].asString();
		qCon.println(hmsg); delay(comdelay);
		DebugPrintln(hmsg);
		hmsg = String("/set?pc2=") + root["p2a"].asString() + "," + root["p2b"].asString() + "," + root["p2c"].asString() + "," + root["p2r"].asString() + "," + root["p2trm"].asString();
		qCon.println(hmsg); delay(comdelay);
		DebugPrintln(hmsg);
		hmsg = String("/set?pc3=") + root["p3a"].asString() + "," + root["p3b"].asString() + "," + root["p3c"].asString() + "," + root["p3r"].asString() + "," + root["p3trm"].asString();
		qCon.println(hmsg); delay(comdelay);
		DebugPrintln(hmsg);


		hmsg = String("/set?al=") + String(Probes[0].AlarmLo) + "," + String(Probes[0].AlarmHi) + "," +  // HMGlobal.hmAlarmLo[1] + "," + HMGlobal.hmAlarmHi[1] + "," + HMGlobal.hmAlarmLo[2] + "," + HMGlobal.hmAlarmHi[2] + "," + HMGlobal.hmAlarmLo[3] + "," + HMGlobal.hmAlarmHi[3];
			String(Probes[1].AlarmLo) + "," + String(Probes[1].AlarmHi) + "," +
			String(Probes[2].AlarmLo) + "," + String(Probes[2].AlarmHi) + "," +
			String(Probes[3].AlarmLo) + "," + String(Probes[3].AlarmHi);
		qCon.println(hmsg); delay(comdelay);
		DebugPrintln(hmsg);


		qCon.println("/set?tt=Web Settings,Updated!!"); delay(comdelay);
		qCon.println("/save?"); delay(comdelay);		
	}  //open file success
	loadProbes(); //reload probe structs in memory
}

void HMGlobalClass::ConfigAlarms(String msgStr, bool Async)
{
	//format is $ALARM,10,20,30,40,50,60,70,80   (lo/hi pairs);  send to comport;
	msgStr.replace("$ALARM,", "");  //remove the alarm command and send rest to HM
	qCon.println("/set?al=" + msgStr); //delay(comdelay);
	DebugPrintln("setting new alarms " + msgStr);
	qCon.println("/set?tt=Web Alarms,Updated..");// delay(comdelay);
	int fy = 0;
	for (int fx = 0; fx < MAX_PROBES; fx++) {
		Probes[fx].AlarmLo = String(getValue(msgStr, fy)).toInt(); fy++;
		Probes[fx].AlarmHi = String(getValue(msgStr, fy)).toInt(); fy++;
	}
	SaveProbes();//save to file
}

void HMGlobalClass::ResetAlarms()
{
	ResetTimeCheck = 0;
	qCon.println("/set?al=0,0,0,0,0,0,0,0"); //delay(comdelay);
}
