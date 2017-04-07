// LogDataClass.h

#ifndef _LOGDATACLASS_h
#define _LOGDATACLASS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "globals.h"

#define DATA_INTERVAL    6    //# of seconds to store records to memory  (every 6 seconds);
#define MAX_MEMRECS     50   //6 second records for 5 minutes is for memory....  when recs are full, we append to log file...
#define STORE_INTERVAL  10   //store/avg every 10 records when saving to log.  must be a perfect multiple of MAX_MEMRECS





class AVRLogDataClass
{
protected:	
	unsigned long lastUpdate;

	String logFName = "/curCook.log";
	int CurRecPos = 0;  //store location of memarrary
	curDataStruct curData[MAX_MEMRECS];  //stores curData in Memory (5 mintues for 5 seconds);
public:
	AVRLogDataClass();
	void LogData(curDataStruct curLogData);
	String GetCurGraphData();
	void begin();  
	void resetLog();

};

extern AVRLogDataClass AVRLogData;

#endif

