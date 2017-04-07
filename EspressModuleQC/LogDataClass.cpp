// 
// 
// 

#include "LogDataClass.h"
#include "myWebServerAsync.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>


String stringIFY(curDataStruct curData)
{
	char lData[200];
	sprintf(lData, "%i,%i,%i,%i,%i,%i,%i,%i,%i\n", curData.readTime, curData.SetPoint, curData.PitTemp, curData.Food1, curData.Food2, curData.Food3, curData.Fan, curData.FanAvg, curData.LidCountDown);	
	return String(lData);
}



AVRLogDataClass::AVRLogDataClass()
{
	lastUpdate = millis() - DATA_INTERVAL * 1001;
}

void AVRLogDataClass::LogData(curDataStruct curLogData)
{
	

	if (millis() - lastUpdate >= DATA_INTERVAL * 1000) {  //if interval passed we store the curdata in memory array....
		lastUpdate = millis();		
		curData[CurRecPos] = curLogData;
		CurRecPos += 1;
		if (CurRecPos == MAX_MEMRECS) {  //if we max memory, we need to append to logfile;				
			int lognum = MAX_MEMRECS / STORE_INTERVAL;
			curDataStruct lData[lognum];  //we take the detail curData array and avg/squeeze into log storage array		
			
			File file = SPIFFS.open(logFName, "a");  //append or create if not exists;
			if (file) {
				
		

				//lets squeeze and avg the memory list;
				for (int lpos = 0, cpos=0; lpos < lognum; lpos += 1, cpos += STORE_INTERVAL) {    //for every interval record we sum intervals and then avg them.
					for (int fx = 0; fx < STORE_INTERVAL; fx += 1) {
						lData[lpos].readTime =  curData[cpos].readTime;
						lData[lpos].SetPoint += curData[cpos + fx].SetPoint;
						lData[lpos].PitTemp  += curData[cpos + fx].PitTemp;
						lData[lpos].Food1    += curData[cpos + fx].Food1;
						lData[lpos].Food2    += curData[cpos + fx].Food2;
						lData[lpos].Food3    += curData[cpos + fx].Food3;
						lData[lpos].Fan      += curData[cpos + fx].Fan;
						lData[lpos].FanAvg   += curData[cpos + fx].FanAvg;
						lData[lpos].LidCountDown += curData[cpos + fx].LidCountDown;						
					}
					lData[lpos].SetPoint /= STORE_INTERVAL;
					lData[lpos].PitTemp /= STORE_INTERVAL;
					lData[lpos].Food1 /= STORE_INTERVAL;
					lData[lpos].Food2 /= STORE_INTERVAL;
					lData[lpos].Food3 /= STORE_INTERVAL;
					lData[lpos].Fan /= STORE_INTERVAL;
					lData[lpos].FanAvg /= STORE_INTERVAL;
					lData[lpos].LidCountDown /= STORE_INTERVAL;									
				}  //end squeeze and avg records.

				for (int lpos = 0; lpos < lognum; lpos += 1) {    //for every interval record we store it in log;		
					//file.printf("%i,%i,%i,%i,%i,%i,%i,%i,%i\n", lData[lpos].readTime, lData[lpos].SetPoint, lData[lpos].PitTemp, lData[lpos].Food1, lData[lpos].Food2, lData[lpos].Food3, lData[lpos].Fan, lData[lpos].FanAvg, lData[lpos].LidCountDown);				
					String tmp = stringIFY(lData[lpos]);
					file.print(tmp);					
				}
				file.close();
			}
			CurRecPos = 0; //reset memcounter;
			DebugPrintln("Wrote Log");
		}
		




	}
}

String AVRLogDataClass::GetCurGraphData()
{
	String res = "";
	
	//first loop through curpos to end to get  history
	for (int lpos = CurRecPos; lpos < MAX_MEMRECS; lpos += 1) {  
		if (curData[lpos].readTime>0) res += stringIFY(curData[lpos]);
	}
    //then we get the remainder so that it is in order....the array is wrap around array, store current position in currecpos
	for (int lpos = 0; lpos < CurRecPos; lpos += 1) {	
		if (curData[lpos].readTime>0) res += stringIFY(curData[lpos]);
	}	
	//request->sendHeader("Access-Control-Allow-Origin", "*");
	//request->send(200, "text/plain", res);	
	return res;
}

void AVRLogDataClass::begin()  //start log class, ensure that log file is not too big
{

 //will check if log is too big and delete it to 60K approx size....safety, as start cook will reset log file
	File file = SPIFFS.open(logFName, "r");  //append or create if not exists;	
	if (file) {
		if (file.size() > 60000)  //if file size if greater than 60K, remove about 20K for safety.  (500 lines);
		{
			int numlines = (file.size() - 59000) / 40;  //estimate # of lines to remove from history log 40b / line
			if (numlines < 500) numlines = 500;
			for (int fx = 0; fx < numlines; fx++) {
				if (file.available()) file.readStringUntil('\n');
			}  //move pointer to x line...
			File outfile = SPIFFS.open(logFName + ".tmp", "w");   //create output curcook we removed last 500 lines....
			if (outfile) {
				while (file.available())
					outfile.println(file.readStringUntil('\n'));
			}
			outfile.close();
			file.close();

			DebugPrintln("truncated log");
			//rename small log to curcook...
			if (SPIFFS.remove(logFName)) {
				SPIFFS.rename(logFName + ".tmp", logFName);  //
			}
		}
		else file.close();		
	}
}

void AVRLogDataClass::resetLog()
{
	SPIFFS.remove(logFName);
}




AVRLogDataClass AVRLogData;