// 
// 
// 

#include "CookClass.h"
#include "FS.h"
#include <TimeLib.h>
#include <ArduinoJson.h> 
#include <myWebServerAsync.h>
#include "LogDataClass.h"





CookClass::CookClass()
{
	for (int fx = 0; fx < 4; fx++) {
		StartTime[fx]=0;  //0 is curtimer, 3 others are user timers.
		EndTime[fx]=0;
	}
}

void CookClass::begin()  //load cookinfo
{
	String values = "";
	File f = SPIFFS.open(fname, "r");
	if (!f) {
		DebugPrintln("CurCookInfo not found");
	}
	else {  //file exists;
		values = f.readStringUntil('\n');  //read json        		
		f.close();

		DynamicJsonBuffer jsonBuffer;

		JsonObject& root = jsonBuffer.parseObject(values);  //parse 
		if (!root.success())
		{
			DebugPrintln("parseObject() curcookload failed");
			return;
		}
//		Probes[fx].Name = root[cP + "name"].asString();
//		Probes[fx].AlarmLo = String(root[cP + "all"].asString()).toInt();
//		Probes[fx].AlarmHi = String(root[cP + "alh"].asString()).toInt();
		status= String(root["status"].asString()).toInt();
		for (int fx = 0; fx < 4; fx++) {
			StartTime[fx] = String(root["STimer"+String(fx)].asString()).toInt();
			EndTime[fx] = String(root["ETimer" + String(fx)].asString()).toInt();
		}

		//DebugPrintln("loaded cookinfo" + String(status));
	}  //file exists
} //file exists;  

String CookClass::AsJSON()
{
	String values = "";
	DynamicJsonBuffer jsonBuffer;

	JsonObject& root = jsonBuffer.parseObject("{}");  //parse weburl

	root["status"] = status;
	for (int fx = 0; fx < 4; fx++) {
		root["STimer" + String(fx)] = StartTime[fx];
		root["ETimer" + String(fx)] = EndTime[fx];
	}  //for each probe
	   //create file and save; 
	String newvalues;
	root.printTo(newvalues);
	return newvalues;
}

void CookClass::SaveJSON()
{
	    File file = SPIFFS.open(fname, "w");
		if (file) {
			file.println(AsJSON());  //save json data
			file.close();
		}
}

void CookClass::StartCook()
{
	if (!status == 0)  return; //if already started then exit;
	
	status = 1;	
	ResetTimer(0);
	StartTimer(0);
	//reset other timers
	for (int fx = 1; fx < 4; fx++) {
		ResetTimer(fx);
	}
	SaveJSON();
	AVRLogData.resetLog();
}

void CookClass::EndCook()
{
	if (!status == 1) return;  //not started yet so exit;
	for (int fx = 0; fx < 4; fx++) {
		StopTimer(fx);
	}
	status = 0;
	SaveJSON();
}

void CookClass::StartTimer(int timerIndex)
{
	if (timerIndex >= 0 && timerIndex <= 3) {
		if (StartTime[timerIndex] == 0) {  //if timer not already started..
			StartTime[timerIndex] = now();
			EndTime[timerIndex] = 0;
			SaveJSON();
		}
	}

}

void CookClass::StopTimer(int timerIndex)
{
	if (timerIndex >= 0 && timerIndex <= 3) {
		if (StartTime[timerIndex] > 0 && EndTime[timerIndex] == 0) {  //if timer already started..		
			EndTime[timerIndex] = now();
			SaveJSON();
		}
	}
}

void CookClass::ResetTimer(int timerIndex)
{
	if (timerIndex >= 0 && timerIndex <= 3) {
		StartTime[timerIndex] = 0;
		EndTime[timerIndex] = 0;
		SaveJSON();
		}	
}


CookClass CookStatus;
