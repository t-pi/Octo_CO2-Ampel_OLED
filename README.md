# Octopus_CO2-Ampel_Pk
Octopus "CO2-Ampel" board with
* NodeMCU-v2 ESP8266
* Bosch BME680
* Sensirion SCD30

# What does it do?
The sketch
* runs Sensirion SCD30 for CO2 ppm level,
* and Bosch Sensortec BSEC on BME680 for IAQ (indoor air quality), including state save option,
* indicates CO2 / IAQ on Neopixels and
* displays the actual values (+ temperature, humidity) on an Adafruit Featherwing OLED 128x32

Press pushbutton B on OLED upon boot-up to start calibration of SCD30.

Press pushbutton A on OLED to cycle through NeoPixel brightness.

Output can also be followed with more details via Serial Monitor

! Important: In order for BSEC to save the state, the BME680 sensor must have
* had some run-in time
* seen fresh air and
* seen bad air, e.g. with a board marker full of solvent or some alcohol.

Easiest way is to follow serial monitor and watch for accuracy (value in parentheses after IAQ) to reach 3.

## Bonus
Mini-CO2-Ampel_Pk.ino is adapted to the Featherwing SCD30/BME680 installed onto an Adafruit Feather Huzzah32 (ESP32 based) with an OLED 128x64 connected via QWIIC connector.

The Featherwing can be found here:
https://www.tindie.com/products/fablab/co2iaq-wing-neopixelbme680-for-scd30-or-mh-z19/

NeoPixel GPIO is Pin 18 on Feather ESP32, btw...

# Sources
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

## Bonus
As the adaption of platform.txt for ESP32 cannot be found directly in the README from BSEC library, here's the extract from the pull request that worked for me:

Original line, around line number 86 or 87

```
## Combine gc-sections, archives, and objects
recipe.c.combine.pattern="{compiler.path}{compiler.c.elf.cmd}" {compiler.c.elf.flags} {compiler.c.elf.extra_flags} {compiler.libraries.ldflags} -Wl,--start-group {object_files} "{archive_file_path}" {compiler.c.elf.libs} -Wl,--end-group -Wl,-EL -o "{build.path}/{build.project_name}.elf"
```

should become

```
## Combine gc-sections, archives, and objects
recipe.c.combine.pattern="{compiler.path}{compiler.c.elf.cmd}" {compiler.c.elf.flags} {compiler.c.elf.extra_flags} -Wl,--start-group {object_files} "{archive_file_path}" {compiler.c.elf.libs} {compiler.libraries.ldflags} -Wl,--end-group -Wl,-EL -o "{build.path}/{build.project_name}.elf"
```

platform.txt in my case found here: ..\packages\esp32\hardware\esp32\1.0.6


