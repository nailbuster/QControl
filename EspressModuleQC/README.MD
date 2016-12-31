
#Espress Module for Q-Control

This is the module that works on a NodeMCU type device (esp8266 w 4MB spiffs).

It works with the Q-Control PID controller and is connected via tx/rx serial comms.

It allows for internet/cloud updating and configuration of the PID controllers.

You can see setup instructions here:  https://github.com/nailbuster/Q-Control/blob/master/EspressModuleQC/bin/Configure%20of%20QControl.pdf


This uses the Arudino CORE for esp8266 so you must already be setup to program/compile to an esp8266 device within your IDE.  Core is here:  https://github.com/esp8266/Arduino

To compile with arudino IDE you'll need the following libraries already installed:

myWebServer (written by me): https://github.com/nailbuster/myWebServer Note that this library needs other libraries as well!

https://github.com/Imroy/pubsubclient Pussubclient by: Nick O'Leary, Ian Tester imroykun@gmail.com

Also,  you will need to include all the libraries sub-folders within this git into your arduino libraries folder.

Visit my website:  http://bbq.nailbuster.com

Forums:  http://bbq.nailbuster.com/forum

