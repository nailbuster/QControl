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


#ifndef _QGLOBALS_h
#define _QGLOBALS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define QCON_VERSION 0.90

#include <EEPROM.h>


#include "qController.h"
#include "qProbes.h"
#include "qLink.h"
#include "qFan.h"
#include "qPID.h"
#include "qGUI.h"




const int MaxProbes = 4;  //includes pit so 4 means 4 probes 1 pit 3 food
const int QVersion = 1;   //used to store version number for eeprom settings
const byte pitProbeIndex = 0;   //which probe is pit (good for debug and erros)


extern void fBuf(char buf[], int bufsize, const __FlashStringHelper *fmt, ...);

//extern String getValue(String data, int index, char separator);
extern String getValue(const String& data, int index, char separator);
extern char *float2s(float f, char buf[], unsigned int digits);

extern int round5(int a);

extern QControllerClass curQ;  //Main Controller
extern QLinkClass curLink;
extern QFanClass curFan;
extern QPIDClass curPID;
extern QProbeClass curProbes[MaxProbes];
extern QGUIClass QGUI;



#endif

