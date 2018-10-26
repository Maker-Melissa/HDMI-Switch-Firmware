# HDMI-Switch-Firmware
Firmware for my Smart House HDMI Switch

By Maker Melissa
https://makermelissa.com/

Based on firmware originally published by MK-Smarthouse.com and republished with permission. For some more Smarthouse stuff and to find some great tutorials check out https://www.mksmarthouse.com/.

## Required Libraries
To compile, you need the following Libraries
* ArduinoJson
* MQTT by Joel Gaehwiler
* WiFiManager

## Required Board
To install, add http://arduino.esp8266.com/stable/package_esp8266com_index.json to Additional Boards Manager URLs under Preferences.
* ESP8266 Board

## Updating Wirelessly (after you have it configured)
I like make the upload Verbose under Preferences, copy the file name ending with .ino.bin mentioned in the output right before it errors over to a non-temp directory. After the initial upload and after configuring to get on WiFi, go to http://[Name you gave Board]/firmware, type in the username and password you gave it.  Select that file from you copied and upload it. I believe there are less messy ways to do this, but it doesn't require any extra software.

## Schematic
![HDMI Switch Circuit.png](HDMI%20Switch%20Circuit.png?raw=true)
