# libm8r - A library for ESP with a Mac simulator

See https://github.com/cmarrin/m8rscript/wiki for an overview of m8rscript and its features.

##Building

Mac uses xcode. Simply open the mac/libm8r.xcodeproj file and build the m8rmac target to get an executable that runs a sample native task

ESP uses the ESP RTOS SDK and is designed to run on the Wemos D1 Mini, although it's not hard to get it running on other models. Instructions on how to set all that up will be coming soon. Once that is installed you can go to the esp directory and type:

~~~~
make -j40 flash monitor
~~~~

This probably won't work because you need to set the correct port. Instructions on how to do that will be coming soon. Once you've successfully loaded the Wemos will try to connect to Wifi, which will not work because it needs the SSID and password. These will be setup with a WebServer, which is not yet complete. After connecting to Wifi it will run the task, which prints a message and starts blinking the LED. It blinks at 1 second intervals until the Wifi connects, then it switches to 3 second intervals.

###More Later...
