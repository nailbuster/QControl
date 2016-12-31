# Q-Control
Q-Control DIY BBQ Smoker Controller with iOT Cloud Functions

This project is a free/open-source DIY project that allows you control of your BBQ charcoal smoker using a fan and/or servo damper.  This is a NEW!!! project for 2017....feel free to help test/update/improve the project.  My website:  http://bbq.nailbuster.com

Q-Control was designed to be used on many different types of AVRs.  It is using the Arduino IDE/Core so should work on many different AVRs moving forward. I've compiled (but not completly tested) on atmega328, MEGA2560, stm32 (maple mini), esp8266....

    Features include:
        1x Pit Probe 
        3x Food probes
        Any/All probes can be either ThermoCouple(Max6675) or Thermistor Types(ThermoWorks Tx series recommended)
        Easy to configure via a web interface.
        Dashboard feature with history graphing of your current cook.
        Remote Temperature Settings
        Alarms (Hi/Low) with notifications.
        and more....
    
Two parts to this project:  

1> Q-Control is the PID controller that runs on the AVR.  This code doesn't interact with the internet.

2> Q-Control Espress Module.  This is the wifi-internet connected device (esp8266 currently) that interacts with the cloud and allows easy configuration via webpages and such.  The sub-folder in this github has the source for the Module.

The two device interact via a simple serial tx/rx connection.  The Espress Module firware can be updated via OTA(wifi) and in some cases the Espress Module can also flash the AVR.

#The Q-Control PID controller:

The code should be straightfoward to understand:  

Simple Objects Interaction/Definition

Main Controller Object:  qController.h/cpp

    QControllerClass		curQ;  //Main Controller
 
    QLinkClass			curLink;  //Link to external esp8266 for web/cloud usually serial
 
    QFanClass			curFan;   //Fan controller and Servo controller
 
    QPIDClass			curPID;   //PID object that will calculate what FAN output should be.
 
    QProbeClass			curProbes[MaxProbes];  //Probes objects, gets temperature support ADC, max6675..etc.
 
    QGUIClass			QGUI;     //GUI menu/led/lcd headless for now, does nothing...  
 

.run method is called on most objects (loop) method.  

More info and discussion at my website:  http://bbq.nailbuster.com
Visit my forums at:  http://bbq.nailbuster.com/forum





                        NO WARRANTY


	
	   BECAUSE THE LIBRARY IS LICENSED FREE OF CHARGE, THERE IS NO
	WARRANTY FOR THE LIBRARY, TO THE EXTENT PERMITTED BY APPLICABLE LAW.
	EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR
	OTHER PARTIES PROVIDE THE LIBRARY "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
	PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
	LIBRARY IS WITH YOU.  SHOULD THE LIBRARY PROVE DEFECTIVE, YOU ASSUME
	THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
	
	  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN
	WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY
	AND/OR REDISTRIBUTE THE LIBRARY AS PERMITTED ABOVE, BE LIABLE TO YOU
	FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR
	CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE
	LIBRARY (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING
	RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A
	FAILURE OF THE LIBRARY TO OPERATE WITH ANY OTHER SOFTWARE), EVEN IF
	SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
	DAMAGES.



    
    
    
