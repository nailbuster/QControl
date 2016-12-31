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

#include <EEPROM.h>
#include "qGlobals.h"
#include <myMax6675.h>
#include <SPI.h>
#include <Servo.h> 


byte me;


/*
Simple Objects Interaction/Definition

Main Controller Object:  qController.h/cpp

extern QControllerClass		curQ;  //Main Controller
extern QLinkClass			curLink;  //Link to external esp8266 for web/cloud usually serial
extern QFanClass			curFan;   //Fan controller and Servo controller
extern QPIDClass			curPID;   //PID object that will calculate what FAN output should be.
extern QProbeClass			curProbes[MaxProbes];  //Probes objects, gets temperature support ADC, max6675..etc.
extern QGUIClass			QGUI;     //GUI menu/led/lcd headless for now, does nothing...  

.run method is called on most objects (loop) method.  

*/



void setup() {
Serial.begin(38400); //debug stuffs
delay(10);
Serial.print(F("version")); Serial.println(QCON_VERSION);
curQ.Begin();  //start Q Controller Main Object
}


void loop() {
  curQ.Run();           //Qcontroller   
}

















