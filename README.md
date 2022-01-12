# Octopus_CO2-Ampel_Pk
Octopus "CO2-Ampel" board with
* NodeMCU-v2 ESP8266
* Bosch BME680
* Sensirion SCD30

What does it do?
The sketch
* runs Sensirion SCD30 for CO2 ppm level,
* and Bosch Sensortec BSEC on BME680 for IAQ (indoor air quality), including state save option,
* indicates CO2 / IAQ on Neopixels and
* displays the actual values (+ temperature, humidity) on an Adafruit Featherwing OLED 128x32

Press pushbutton B on OLED upon boot-up to start calibration of SCD30.
Press pushbutton A on OLED to cycle through NeoPixel brightness.

Based on CO2-Ampel-DIY-Octopus-SCD30-Neo-OLEDflickerfree.ino from https://github.com/make-IoT/CO2-Ampel

Octopus CO2-Ampel board, s. here:
https://www.tindie.com/products/fablab/diy-octopus-with-scd30-and-nodemcu-v2/

The BSEC library is (c) by Robert Bosch GmbH / Bosch Sensortec GmbH and is available here:
https://www.bosch-sensortec.com/software-tools/software/bsec/
Installation instructions are available here:
https://github.com/BoschSensortec/BSEC-Arduino-library

Tested with BSEC_1.4.8.0 from Arduino library "BSEC Software Library" 1.6.1480

# Installation
* Add "http://arduino.esp8266.com/stable/package_esp8266com_index.json" into additional board URLs in Arduino IDE settings
* Set Arduino IDE to "NodeMCU 1.0 (ESP-12E Module)" and your COM port
* Install required libraries
  * Adafruit NeoPixel for NeoPixel LEDs
  * Adafruit SSD1306 for OLED display
  * Adafruit GFX for nicer number display
  * Sparkfun SCD30 Arduino library for Sensirion CO2 sensor
  * BSEC Software Library
* Follow instructions on BSEC Software Library home page: https://github.com/BoschSensortec/BSEC-Arduino-library to modify platform.txt for esp8266 (in my case located at ...\packages\esp8266\hardware\esp8266\3.0.2\platform.txt)
* Add further libraries I might have forgotten and Arduino IDE complains about :) Be sure to enable verbose compiling and uploading in Arduino IDE settings

Output can also be followed via Serial Monitor
