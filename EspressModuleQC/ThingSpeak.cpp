/*
ThingSpeak.cpp QControl Connection for esp8266

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


#include "ThingSpeak.h"
#include "globals.h"
#include <myWebServerAsync.h>
#include <ArduinoJson.h> 
#include <FS.h>
#include <ESP8266WiFi.h>
#include "ESPAsyncTCP.h"
#include "SyncClient.h"
#include "ESP8266HTTPClient.h"




ThingSpeakClass ThingSpeak;

ThingSpeakClass::ThingSpeakClass()
{
	lastThingSpeak = millis();
	lastTalkBack = millis();
	ThingEnabled = false;  //defaults;
	TalkBackEnabled = false;
	thingInterval = 15;
	talkBackInterval = 15;  //in seconds
}

void ThingSpeakClass::begin()  //loads settings from json file....
{
	String values = "";

	File f = SPIFFS.open("/cloudgen.json", "r");
	if (!f) {
		DebugPrintln("thingspeak config not found");
	}
	else {  //file exists;
		values = f.readStringUntil('\n');  //read json        		
		f.close();

		DynamicJsonBuffer jsonBuffer;

		JsonObject& root = jsonBuffer.parseObject(values);  //parse weburl
		if (!root.success())
		{
			DebugPrintln("parseObject() thingspeak failed");
			return;
		}
		if (root["spkurl"].asString() != "") { //verify good json info                                                
			thingSpeakURL = root["spkurl"].asString();
			thingWriteKey = root["spkwkey"].asString();
			thingInterval = String(root["spkint"].asString()).toInt();
			TalkBackID = root["tkbid"].asString();
			TalkBackKey = root["tkbkey"].asString();
			talkBackInterval = String(root["tkbint"].asString()).toInt();
			if (String(root["status"].asString()).toInt() == 1) ThingEnabled = true; else ThingEnabled = false;
			if (String(root["tbstatus"].asString()).toInt() == 1) TalkBackEnabled = true; else TalkBackEnabled = false;
			DebugPrintln("ThingSpeak Starting....");
		}
	} //file exists;        
	
}

void  ThingSpeakClass::handle()
{
	if (ThingEnabled == false) return;

	unsigned long curTime = millis();

	//manage timers manually....more stable this way!
	if (curTime - lastTalkBack >= talkBackInterval * 1000) {
		ProcessTalkBacks();
		lastTalkBack = millis();
	}
	if (curTime - lastThingSpeak >= thingInterval * 1000) {
		SendThingSpeakValues();
		lastThingSpeak = millis();
	}

}

void ThingSpeakClass::ProcessTalkBacks()
{
	String msgStr = "";
	if (TalkBackEnabled == false) return;
	if (thingSpeakURL == "") return;

	msgStr = getThingSpeak(TalkBackID, TalkBackKey);
	if (msgStr.charAt(0) == '$') {   //process talkback as a command
		
		if ((avrGlobal.getValue(msgStr, 0) == "$ALARM"))
		{
			avrGlobal.ConfigAlarms(msgStr);
		}
		if ((avrGlobal.getValue(msgStr, 0) == "$SETPOINT"))
		{
			avrGlobal.SetTemp(String(avrGlobal.getValue(msgStr,1)).toInt());
		}
	}

}

String ThingSpeakClass::getThingSpeak(String talkBackID, String talkApiKey)   //talkback message processing
{  //TalkBack Function from thingspeak
	String pageLength;
	String CommandString = "";
	String HTMLResult;
	bool GoodResult = false;

	DebugPrintln("talkback checking...");
	if (TalkBackEnabled == false) return "";
	if (thingSpeakURL == "") return "";
	

	if (WiFi.status() != WL_CONNECTED) return "";   //check to make sure we are connected...

	String url = "/talkbacks/" + talkBackID + "/commands/execute?api_key=" + talkApiKey;

/*  //todo sync client WIP	
	SyncClient client;
	String SrvURL = thingSpeakURL;
	String GetST = "GET " + url + " HTTP/1.1\r\nHost: " + thingSpeakURL + "\r\nConnection: close\r\n\r\n";
	if (!client.connect(SrvURL.c_str(), 80)) {
		DebugPrintln("Connect Failed");
		return "";
	}
	client.setTimeout(2);
	if (client.printf(GetST.c_str()) > 0) {
		while (client.connected() && client.available() == 0) {
			delay(1);
		}
		while (client.available()) {
			//CommandString+=client.read();
			Serial.write(client.read());
		}
		if (client.connected()) {
			client.stop();
		}
	}
	else {
		client.stop();
		DebugPrintln("Send Failed");
		while (client.connected()) delay(0);
	}

*/
	
	HTTPClient http;	

	http.begin("http://"+ thingSpeakURL + url);

	int httpCode = http.GET();

	// httpCode will be negative on error
	if (httpCode > 0) {
		// HTTP header has been send and Server response header has been handled
		//DebugPrintln("[HTTP] GET... code: " + String(httpCode));
		// file found at server
		if (httpCode == HTTP_CODE_OK) {
			CommandString  = http.getString();			
		}
	}
	else {
		DebugPrintln("[HTTP] GET... failed, error: " + http.errorToString(httpCode));
	}   
	http.end();

	
	
	CommandString.replace("\n", "");
	if (CommandString!="") DebugPrintln("Got talkback result : " + CommandString);
	return CommandString;
	
}




void ThingSpeakClass::SendThingSpeakValues()
{

	//  if (ThingEnabled) DebugPrintln("thingspeak enabled"); else DebugPrintln("thingspeak disabled");                   
	if (ThingEnabled == false) return;
	if (thingSpeakURL == "") return;
	DebugPrintln(thingSpeakURL);

	if (WiFi.status() != WL_CONNECTED) return;   //check to make sure we are connected...	  

	String postStr = "api_key=" + thingWriteKey;
	if (avrGlobal.avrPitTemp != "U")  postStr += "&field1=" + avrGlobal.avrPitTemp;
	if (avrGlobal.avrFood1 != "U") postStr += "&field2=" + avrGlobal.avrFood1;
	if (avrGlobal.avrFood2 != "U") postStr += "&field3=" + avrGlobal.avrFood2;
	if (avrGlobal.avrAmbient != "U") postStr += "&field4=" + avrGlobal.avrAmbient;
	if (avrGlobal.avrFanMovAvg != "U") postStr += "&field5=" + avrGlobal.avrFanMovAvg;
	if (avrGlobal.avrFan != "U") postStr += "&field6=" + avrGlobal.avrFan;
	if (avrGlobal.avrSetPoint != "U") postStr += "&field7=" + avrGlobal.avrSetPoint;
	if (avrGlobal.avrLidOpenCountdown != "U") postStr += "&field8=" + avrGlobal.avrLidOpenCountdown;

	if (inAlarm)  //if alarm was triggered we send on next msg
	{
		postStr += "&status=" + WebServer.urlencode(WebServer.CurTimeString() + " " + AlarmInfo);
		AlarmInfo = "";
		inAlarm = false;
	}

/* TODO SYNC CLIENT	
	String SrvURL = thingSpeakURL;
	SyncClient client;
	if (!client.connect(SrvURL.c_str(), 80)) {
		DebugPrintln("Connect Failed");
		return;
	}
	client.setTimeout(2);
	String OutST = "POST /update HTTP/1.1\r\n" +
		String("Host: ") + String(thingSpeakURL) + "\r\n" +
		"Cache-Control: no-cache\r\n" +
		"Content-Type: application/x-www-form-urlencoded\r\n" +
		"Content-Length: " + String(postStr.length()) + "\r\n\r\n" +
		postStr + "\r\n";		  	 
	 if (client.printf(OutST.c_str()) > 0) {
		while (client.connected() && client.available() == 0) {
			delay(1);
		}
		while (client.available()) {
			//CommandString += client.read();
			Serial.write(client.read());
		}
		if (client.connected()) {
			client.stop();
		}
	}
	else {
		client.stop();
		DebugPrintln("Send Failed");
		while (client.connected()) delay(0);
	}
*/

	
		HTTPClient http;
		
		DebugPrintln("http://" + thingSpeakURL + "/update");
		http.begin("http://" + thingSpeakURL + "/update");

		int httpCode = http.POST(postStr);

		// httpCode will be negative on error
		if (httpCode > 0) {
			if (httpCode == HTTP_CODE_OK) {				
			}
		}
		else {
			DebugPrintln("[HTTP] POST... failed, error: " + http.errorToString(httpCode));
		}
		http.end(); 
		
		
	DebugPrintln("sending thingspeak stuffs");

}

/*  do this directly in javascript :)
String ThingSpeakClass::CreateThingSpeakChannel(String apikey)
{
	String httpRES = "";
	if (thingSpeakURL == "") thingSpeakURL="api.thingspeak.com"; //defalt
	DebugPrintln("Creating ThingSpeak Channel");

	if (WiFi.status() != WL_CONNECTED) return "";   //check to make sure we are connected...	  

	String postStr = "api_key=" + apikey;
	postStr += "&name=myQControl";
	postStr += "&field1=PitTemp";
	postStr += "&field2=FoodTemp1";
	postStr += "&field3=FoodTemp2";
	postStr += "&field4=AmbTemp";
	postStr += "&field5=AlarmMsg";
	postStr += "&field6=Fan";
	postStr += "&field7=SetPoint";
	postStr += "&field8=LidOpen";
	

	HTTPClient http;

	DebugPrintln("http://" + thingSpeakURL + "/channels.json");
	http.begin("http://" + thingSpeakURL + "/channels.json");

	int httpCode = http.POST(postStr);

	// httpCode will be negative on error
	if (httpCode > 0) {
		if (httpCode == HTTP_CODE_OK) {
			httpRES = http.getString();
		}
	}
	else {
		DebugPrintln("[HTTP] POST... failed, error: " + http.errorToString(httpCode));		
	}
	http.end();


	DebugPrintln("sending thingspeak create");

	return httpRES;
}
*/
void ThingSpeakClass::SendAlarm(String AlarmMsg)
{
	if (ThingEnabled == false) return;
	if (thingSpeakURL == "") return;
	inAlarm = true;  //next message send we will send alarm info.
	AlarmInfo = AlarmMsg;
	DebugPrintln(thingSpeakURL);	
}
