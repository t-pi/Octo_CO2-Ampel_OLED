/* This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. */
 
/*  Initial version: CO2-Ampel-DIY-Octopus-SCD30-Neo-OLEDflickerfree.ino from https://github.com/make-IoT/CO2-Ampel
 *  2022-01-10: V2.0 adapted by t-pi:
 *  - removed support for CharliePlex Featherwing
 *  - removed ticker and adjusted loop to react to BSEC
 *  - removed support for separate OLED 128x64
 *  - added support for Featherwing OLED 128x32
 *  - improved color coding of NeoPixels (shift from green to yellow to red, switch to violet >1500 ppm
 *  - added state saving for BSEC library (to avoid long initialization phase)
 *  - several small adjustments
 */
#include <EEPROM.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBoldOblique12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <SparkFun_SCD30_Arduino_Library.h>
/* 
 Bosch BSEC Lib, https://github.com/BoschSensortec/BSEC-Arduino-library
 The BSEC software is only available for download or use after accepting the software license agreement.
 By using this library, you have agreed to the terms of the license agreement: 
 https://ae-bst.resource.bosch.com/media/_tech/media/bsec/2017-07-17_ClickThrough_License_Terms_Environmentalib_SW_CLEAN.pdf */
#include <bsec.h>

/* Configure the BSEC library with information about the sensor
    18v/33v = Voltage at Vdd. 1.8V or 3.3V
    3s/300s = BSEC operating mode, BSEC_SAMPLE_RATE_LP or BSEC_SAMPLE_RATE_ULP
    4d/28d = Operating age of the sensor in days
    generic_18v_3s_4d
    generic_18v_3s_28d
    generic_18v_300s_4d
    generic_18v_300s_28d
    generic_33v_3s_4d
    generic_33v_3s_28d
    generic_33v_300s_4d
    generic_33v_300s_28d
*/
const uint8_t bsec_config_iaq[] = {
#include "config\generic_33v_3s_4d\bsec_iaq.txt"
};

#define STATE_SAVE_PERIOD  UINT32_C(360 * 60 * 1000) // 360 minutes - 4 times a day

// Helper functions declarations
void checkIaqSensorStatus(void);
void loadState(bool init);
void updateState(void);

Bsec iaqSensor;     // Create an object of the class Bsec 
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
uint16_t stateUpdateCounter = 0;
#define TEMPERATURE_CORRECTION 5

//Reading CO2, humidity and temperature from the SCD30 By: Nathan Seidle SparkFun Electronics 
//https://github.com/sparkfun/SparkFun_SCD30_Arduino_Library
SCD30 airSensorSCD30; // Objekt SDC30 Umweltsensor

//Adafruit_NeoPixel WSpixels = Adafruit_NeoPixel((1<24)?1:24,15,NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2,13,NEO_GRBW + NEO_KHZ800);
#define NEO_IAQ 0
#define NEO_CO2 1

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);  // Featherwing OLED
GFXcanvas1 canvas(128,32);
// OLED FeatherWing buttons map to different pins depending on board:
#define BUTTON_A  0
#define BUTTON_B 16
#define BUTTON_C  2


void setup(){
  EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1); // 1st address for the length
  Serial.begin(115200);
  WiFi.forceSleepBegin(); // Wifi off
  delay(50);

  // --- Init I2C-Bus
  Wire.begin(); 
  Wire.setClock(100000L);            // 100 kHz SCD30 
  Wire.setClockStretchLimit(200000L);// CO2-SCD30
  if (Wire.status() != I2C_OK) Serial.println("Something wrong with I2C");

  // --- Init SCD30
  Serial.println("SCD30 init... ");
  if (airSensorSCD30.begin() == false) {
    Serial.println("The SCD30 did not respond. Please check wiring."); 
    while(1) {
      yield(); 
      delay(1);
    } 
  }
  airSensorSCD30.setAutoSelfCalibration(false); // Sensirion no auto calibration
  airSensorSCD30.setMeasurementInterval(2);     // CO2-Messung alle 2 s

  // --- Init NeoPixels
  pixels.begin();
  delay(1);
  pixels.show();
  pixels.setPixelColor(NEO_IAQ,30,30,30,30); // switch on
  pixels.setPixelColor(NEO_CO2,30,30,30,30);
  pixels.show();                
  delay(100);
  pixels.setPixelColor(NEO_IAQ,0,0,0,0); // ...and off
  pixels.setPixelColor(NEO_CO2,0,0,0,0);
  pixels.show(); 

  // --- Init OLED display
  Serial.println("OLED init...");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C default, Featherwing OLED
  pinMode(BUTTON_C, INPUT);

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("CO2 Ampel V22/1 Pk");
  display.println("starting ...");
  display.println("");
  display.display();
  delay(2000);

  // --- Init BME680 and BSEC
  Serial.print("BME680 init... ");
  delay(200);
  iaqSensor.begin(BME680_I2C_ADDR_PRIMARY, Wire);
  String output = "BSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  Serial.print("BSEC: config... ");
  iaqSensor.setConfig(bsec_config_iaq);
  checkIaqSensorStatus();

  Serial.print("recover last state... ");
  loadState();

  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  Serial.print("subscribe... ");
  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();
  
  if (iaqSensor.run()) {
    Serial.println("ok");
    Serial.println("BME data: " + String(iaqSensor.temperature) + "C, " + String(iaqSensor.humidity));
  } else {
    Serial.println("NOT OK!");
  }

  // --- OLED Pushbutton C: Force Calibration of Sensirion SCD 30
  if (!(digitalRead(BUTTON_C)))
  {
    Serial.println("Manual calibration of SCD30 CO2, please provide fresh air");
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("SCD30 CO2 Calibration?");
    display.println("Hold button(C) for 5 sec");
    display.display();
    
    delay( 5000 );
    if (!( digitalRead(BUTTON_C) )) {
      int i = 0;
      int sensor = 0;
      Serial.print("Start SCD 30 calibration, please wait ...");
      for (i= 1; i<= ( 24 ); i=i+1) {
        sensor = airSensorSCD30.getCO2() ;
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("CO2: " + String(sensor) + " ppm");
        display.println("Progress: "+String((i)*5)+"/120s");
        display.display();
        delay(5000);
        yield();
      }
      airSensorSCD30.setAltitudeCompensation(0); // Altitude in m ü NN 
      airSensorSCD30.setForcedRecalibrationFactor(420); // fresh air 
      delay(1000);
    }
  }
  display.clearDisplay();
  display.display();
  Serial.println("Setup done.");
}


void loop() { // Kontinuierliche Wiederholung 
  int CO2 = 0;  
  float sTEMP = 0;
  float sHUM = 0;
  int IAQ = 0;
  float bTEMP = 0;
  float bHUM = 0;
  
  if (iaqSensor.run()) {
    IAQ = iaqSensor.iaq;
    bTEMP = iaqSensor.temperature - TEMPERATURE_CORRECTION;
    bHUM = iaqSensor.humidity;
    updateState();
    CO2 = airSensorSCD30.getCO2();
    sTEMP = airSensorSCD30.getTemperature() - TEMPERATURE_CORRECTION;
    sHUM = airSensorSCD30.getHumidity();
    
    canvas.fillScreen(SSD1306_BLACK);
    canvas.setTextSize(1);
    canvas.setTextColor(SSD1306_WHITE);
    canvas.setFont(&FreeMonoBold12pt7b);
    canvas.setCursor(0,14);
    canvas.print(String(IAQ));
    if (CO2 < 1000)
      canvas.setCursor(79,14);
    else
      canvas.setCursor(64,14);
    if (CO2 <= 9999) 
      canvas.print(String(CO2));
    else
      canvas.print("!!!!");
    canvas.setFont();
    canvas.setCursor(0,16);
    canvas.print("IAQ");
    canvas.setCursor(100,16);
    canvas.print("ppm");
    canvas.setCursor(21,24);
    canvas.print(String(sTEMP) + "C | ");
    canvas.print(String(int(sHUM))+"%");
      
    display.drawBitmap (0,0, canvas.getBuffer(), 128, 32, SSD1306_WHITE, SSD1306_BLACK);
    display.display();
  
    Serial.print("CO2: " + String(CO2));
    Serial.print(" / IAQ: " + String(IAQ) + " (" + String(iaqSensor.iaqAccuracy) + ")");
    Serial.print(" / VOC: " + String(iaqSensor.gasResistance));
    Serial.print(" / Temp (bme/sens): " + String(bTEMP) + " / " + String(sTEMP));
    Serial.println(" / Hum (bme/sens): " + String(bHUM) + " / " + String(sHUM));
    check_neo(CO2, IAQ);
  } else {
    checkIaqSensorStatus();
  }
  delay(200);
}

long get_CO2color(int CO2) {
    switch (CO2) {
      case 0 ... 700:
        // 0xGGRRBB
        return 0xFF0000; // green
      case 701 ... 956:
        return (0xFF0000+((CO2-701)<<8)); // shift to yellow
      case 957 ... 1211:
        return ((0xFFFF00-((CO2-957)<<16))); // shift to red
      case 1212 ... 1500:
        return (0x00FF00); // red
      default:
        return (0x00FFFF); // violet >1500 ppm
    }
}

void check_neo(int CO2, int IAQ) {
  long color = get_CO2color(CO2); // 0xGGRRBB
  int g = color >> 16;
  int r = color >> 8 & 0xFF;
  int b = color & 0xFF;
  pixels.setPixelColor(NEO_CO2,r,g,b,0);
  pixels.show();
  
  color = get_CO2color(int(float(IAQ)/250.0*1200)); // 0xGGRRBB
  g = color >> 16;
  r = color >> 8 & 0xFF;
  b = color & 0xFF;
  pixels.setPixelColor(NEO_IAQ,r,g,b,0);
  pixels.show();
}

void checkIaqSensorStatus(void) { 
  String output; 
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      display.clearDisplay();
      display.setCursor(0,0);
      display.println(output);
      display.display();
      for (;;) {
        Serial.println(output);
        delay(1000);
      } // Halt in case of failure 
    } 
    output = "BSEC warning code : " + String(iaqSensor.status);
    Serial.println(output);
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      display.clearDisplay();
      display.setCursor(0,0);
      display.println(output);
      display.display();
      for (;;){
        Serial.println(output);
        delay(1000);
      }  // Halt in case of failure 
    } 
    output = "BME680 warning code : " + String(iaqSensor.bme680Status);
    Serial.println(output);
  }
}


void loadState() {
  if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE) {
    // Existing state in EEPROM
    Serial.println("Reading state from EEPROM");
    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
      bsecState[i] = EEPROM.read(i + 1);
      Serial.print(bsecState[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    iaqSensor.setState(bsecState);
    checkIaqSensorStatus();
  } else {
    // Erase the EEPROM with zeroes
    Serial.println("Erasing EEPROM");
    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE + 1; i++)
      if (EEPROM.read(i) != 0)
        EEPROM.write(i, 0);
    EEPROM.commit();
  }
}

void updateState(void)
{
  bool update = false;
  if (stateUpdateCounter == 0) {
    /* First state update when IAQ accuracy is >= 3 */
    if (iaqSensor.iaqAccuracy >= 3) {
      update = true;
      stateUpdateCounter++;
    }
  } else {
    /* Update every STATE_SAVE_PERIOD minutes */
    if ((stateUpdateCounter * STATE_SAVE_PERIOD) < millis()) {
      update = true;
      stateUpdateCounter++;
    }
  }

  if (update) {
    iaqSensor.getState(bsecState);
    checkIaqSensorStatus();
    Serial.println("Writing state to EEPROM");
    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE ; i++) {
      if (EEPROM.read(i + 1) != bsecState[i]) {
        EEPROM.write(i + 1, bsecState[i]);
      }
      Serial.print(bsecState[i], HEX);
      Serial.print(" ");
    }
    EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
    EEPROM.commit();
    Serial.println();
  }
}
