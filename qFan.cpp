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

#include "qFan.h"
#include "qGlobals.h"
#include <Servo.h>





QFanClass::QFanClass()
{
}

void QFanClass::SetFanSpeed(byte fanSpeed)  //fanspeed is 0-255....
{
	int fanDuty = 0;  //local fanspeed 0-255
					  // fanduty is determined by fanStartDuty (duty below is servo only),  once above then we get proportion for fanspeed...
					  // so fanStartDuty=100 (0-255),  that means servo will go from servoLO to ServoHi between 0-99, at 100 servo will be servoHI and then Fan will Start over 100-255....

	curDuty = fanSpeed;
	
	int maxPWM = maxSpeed / 100.0 * hardwarePWM;
	int minPWM = minSpeed /100.0 * hardwarePWM;
	int maxStartPWM = maxStart /100.0 * hardwarePWM;

	//safety!!
	if (curQ.pit_temp >= (curQ.setpoint+25)) curDuty = 0;  //ensure we don't go crazy!
	if (curQ.NoPitt) curDuty = 0; //pitProbeIndex is pit probe; 0 if disconnected.
	
	float alpha = 0.05; // factor to tune	
	runningAvg = (alpha * curDuty) + (1 - alpha) * runningAvg;

	SetServoPos();    //will set Servo Position based on CurDuty...

	if (abs(runningAvg - curDuty) > 30) { runningAvg = curDuty; }  //if big change then don't use avg....

	if (fanStartDuty == hardwarePWM) fanDuty = 0;  //no fan setting (only Servo)
	else if (curDuty < fanStartDuty) fanDuty = 0;  //only set Servo (no fan yet);
	else fanDuty = ((((runningAvg -fanStartDuty)*1.00) / (hardwarePWM - fanStartDuty)) * (maxPWM - minPWM))+minPWM;  //convert % open of fan to PWM (between MIN-MAX PWM);	

    //we have feature MinTempFan that will not blow fan on a very low fire....don't want to blow startup flame....
	if (curQ.pit_temp <= curQ.MinTempFan) fanDuty = 0;
	
	if (curQ.targetReached == false)  //preheat
	{
		if (fanDuty > maxStartPWM) fanDuty = maxStartPWM;
	}
	
	if (fanDuty > hardwarePWM) fanDuty = hardwarePWM;
	else if (fanDuty < 0) fanDuty = 0;

	


	if (fanHasPWM)    //if pwm or curDuty >minpwm....else pulse mode gpio.
	{
		analogWrite(gpioPin, fanDuty);
		//Serial.print("fan:"); Serial.println(fanDuty);
		RelayStartTime = millis();  //stored for swap of pwm and pulse mode....
	}
	else   //no pwm so we pulse based on MAXPWM window size.
	{
		
		unsigned long now = millis();
		if (now - RelayStartTime > relaytime)                     //maxPWM is the time ie 5000 maxpwm is 5s interval for window
		{ //time to shift the Relay Window
			RelayStartTime += relaytime;
		}
		
		float ratio = (fanDuty*1.00) / (hardwarePWM * 1.00);
		if (ratio*relaytime > now - RelayStartTime)
		{
		//	if (fanHasPWM) { analogWrite(gpioPin, minPWM); }
		//	else { digitalWrite(gpioPin, HIGH); }			
			digitalWrite(gpioPin, HIGH);
		}
		else {
		//	if (fanHasPWM) { analogWrite(gpioPin, 0); }
		//	else { digitalWrite(gpioPin, LOW); }			
			digitalWrite(gpioPin, LOW);
		}
	}	
}

void QFanClass::SetServoPos()
{ // when fan starts and servo is at max(servoHI); 0 means noServo
  // so fanStartDuty=100 (0-255),  that means servo will go from servoLO to ServoHi between 0-99, at 100 servo will be servoHI and then Fan will Start over 100....
	int tmpPos = curDuty;
	byte pos;
	int sd = 20; //servodelay

	if (fanStartDuty == 0) exit;  //if no servo attached;


	//convert curDuty into 0-180 degrees for servo

	if (curDuty >= fanStartDuty) tmpPos = ServoHi;  //if duty is over fanstart then servo must be fully open;
	else tmpPos = ((curDuty*1.00 / fanStartDuty) * abs(ServoHi - ServoLow)) + ServoLow;  //convert % open of servo to position 0-180;
	
	if (tmpPos > 180) tmpPos = 180;        //limits to servo microseconds x10
	else if (tmpPos < 0) tmpPos = 0;

	

	//we use running avg of servo positions so that we don't move too often during near setpoint....

	float alpha = 0.05; // factor to tune	
	runningServoAvg = (alpha * tmpPos) + (1 - alpha) * runningServoAvg;  

	if (abs(tmpPos - runningServoAvg)>30) runningServoAvg = tmpPos;  //big moves reset avg....

	tmpPos = floor(runningServoAvg);

	//resoltuion nearest 5 degree;
	if (ServoRound) tmpPos = round5(tmpPos);
	//Serial.print("srv:"); Serial.println(tmpPos);


	if ((millis() - ServoLastTimePos) >(ServoTimeRelease * 1000))  //we detact servo after 3 seconds to avoid 'buzzing/battery'....
	{
		if (myServo.attached())  myServo.detach();
		ServoLastTimePos = millis();
		//Serial.println("servo detach");
	}
		
	//if curposition needs to be moved, attach to servo and move it.

	if (abs(tmpPos - curServoPos) > 0)   // move around if 5 degrees difference
	{   if (myServo.attached() == false) myServo.attach(gpioServoPIN);		
	    myServo.write(tmpPos);
		curServoPos = tmpPos;
		ServoLastTimePos = millis();
//		Serial.println("server moved");
	} 


}

void QFanClass::begin()
{
	pinMode(gpioPin, OUTPUT);  //set fan pin to output
	SetFanSpeed(0);  //turn off fan for safety at start....      
	RelayStartTime = millis();
	//Serial.println("sp" + String(gpioServoPIN));
}

void QFanClass::SetPWMFreq(int FreqIndx)
{    //put a case for this soon....
	//TCCR2B = TCCR2B & B11111000 | B00000001;    // set timer 2 divisor to     1 for PWM frequency of 31372.55 Hz
	//TCCR2B = TCCR2B & B11111000 | B00000010;    // set timer 2 divisor to     8 for PWM frequency of  3921.16 Hz
	//TCCR2B = TCCR2B & B11111000 | B00000011;    // set timer 2 divisor to    32 for PWM frequency of   980.39 Hz
//THIS ONE!!        TCCR2B = TCCR2B & B11111000 | B00000100;    // set timer 2 divisor to    64 for PWM frequency of   490.20 Hz (The DEFAULT)
//	TCCR2B = TCCR2B & B11111000 | B00000101;    // set timer 2 divisor to   128 for PWM frequency of   245.10 Hz
//  TCCR2B = TCCR2B & B11111000 | B00000110;    // set timer 2 divisor to   256 for PWM frequency of   122.55 Hz
//TCCR2B = TCCR2B & B11111000 | B00000111;    // set timer 2 divisor to  1024 for PWM frequency of    30.64 Hz
}

int QFanClass::duty100(bool avg)  //get duty as a percent whole 0-100  curDuty is 0-255;
{
	

	if (avg) return int(runningAvg * 100 / hardwarePWM);
	    else return curDuty * 100 / hardwarePWM;
}


void QFanClass::AsJson(char *jData)
{

	fBuf(jData, 300, F("{fs:%i,pwm:%i,cd:%i,xp:%i,np:%i,pin:%i,int:%i,mt:%i,sl:%i,sh:%i,fm:%i,pm:%i,ms:%i,fd:%i,sp:%i}"),
		fanStatus, fanHasPWM, curDuty, maxSpeed, minSpeed, gpioPin, interval, maxTime, ServoLow, ServoHi, fanHasPWM, pitMin, maxStart,fanStartDuty,gpioServoPIN);

}

void QFanClass::SetValuesJson(const String& msgStr)
{	

	String curPart, curField, curVal;
	for (int fy = 0; fy <= 50; fy++)  //try 50 times
	{
		curPart = String(getValue(msgStr, fy, ','));  //grab first node separated by ,

		if (curPart == "") { break; } //exit if not found

		curField = getValue(curPart, 0, ':');
		curVal = getValue(curPart, 1, ':');

//		Serial.println(curVal);
//      Serial.println(curField);
		if (curField == "fs") { fanStatus = curVal.toInt();}
		else if (curField == "pwm") { fanHasPWM = curVal.toInt(); }
	//	else if (curField == "cd") { curDuty = curVal.toInt(); }
		else if (curField == "xp") { maxSpeed = curVal.toInt(); }
		else if (curField == "np") { minSpeed = curVal.toInt(); }
		else if (curField == "pin") { gpioPin = curVal.toInt(); }
//		else if (curField == "int") { interval = curVal.toInt(); }
//		else if (curField == "mt") { maxTime = curVal.toInt(); }
		else if (curField == "sl") { ServoLow = curVal.toInt(); }
		else if (curField == "sh") { ServoHi = curVal.toInt(); }
//		else if (curField == "fm") { fanHasPWM = curVal.toInt(); }
		else if (curField == "pm") { pitMin = curVal.toInt(); }
		else if (curField == "ms") { maxStart = curVal.toInt(); }
		else if (curField == "fd") { fanStartDuty = curVal.toInt(); }
		else if (curField == "sp") { gpioServoPIN = curVal.toInt(); }  //future hardcode pins for now...
	}
	
}



