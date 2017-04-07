// qcExtras.h

#ifndef _QCEXTRAS_h
#define _QCEXTRAS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


unsigned long ipow10(unsigned power);


char *float2s(float f, char buf[], unsigned int digits);











#endif

