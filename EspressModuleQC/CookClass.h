// CookClass.h

#ifndef _COOKCLASS_h
#define _COOKCLASS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class CookClass
{
 protected:
	 String fname = "/CurCookInfo.json";

 public:
	CookClass();

	int status = 0;
	unsigned long StartTime[4];  //0 is curtimer, 3 others are user timers.
	unsigned long EndTime[4];
	void begin();
	String AsJSON();
	void SaveJSON();
	void StartCook();
	void EndCook();
	void StartTimer(int timerIndex);  // zero based 0-3
	void StopTimer(int timerIndex);
	void ResetTimer(int timerIndex);
};

extern CookClass CookStatus;

#endif

