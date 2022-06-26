
// Basis
#include <Arduino.h>

// MHZ19 Lib
#include "MHZ19.h"       

// BTLE Lib
#include "SimpleBLE.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Display Lib
#include <U8g2lib.h>

//DHT22 Lib
#include "DHTesp.h" 



// MHZ19 SerialPort auswählen
#define MHZ19SERIAL   Serial2
#define MHZ19BAUD     9600

// BTLE Config
#define BTLENAME   "CO2 Sensor"

// DHT COnfig
#define DHTPIN     22


// Display & Font
#define STDFONT      u8g2_font_6x10_tf 
#define STDLOWFONT   u8g2_font_5x8_tf  
#define PPMFONT      u8g2_font_inb24_mr
#define PPMDEFFONT   u8g2_font_6x10_tf
#define TEMPFONT     u8g2_font_6x10_tf
#define INFOFONT     u8g2_font_6x10_tf

#define PPMTOPOFFSET   14
#define PPMLEFTOFFSET   5
#define TEMPTOPOFFSET  55
#define INFOTOPOFFSET  42

// LED & Buzzer
#define BUZZER 27
#define LEDRED 26
#define LEDYEL 25
#define LEDGRN 33

#define PPMNAME    "ppm"

// Global Variables
MHZ19 myMHZ19;
SimpleBLE ble;
DHTesp dht;
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* CS=*/ 5, /* reset=*/ 15);
unsigned long dataTimer = 0;
unsigned int recalibrationTrigger = 0;

void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void setup() {
  // DebugSerial
  Serial.begin(9600);

  // MHZ19 Init
  MHZ19SERIAL.begin(MHZ19BAUD);
  myMHZ19.begin(MHZ19SERIAL);
  myMHZ19.autoCalibration(false);  
  
  // BLE Init
  ble.begin(BTLENAME);

  // LED & Buzzer Init
  pinMode(LEDRED, OUTPUT);
  pinMode(LEDYEL, OUTPUT);
  pinMode(LEDGRN, OUTPUT);
  pinMode(BUZZER, OUTPUT); 

  // DHT Init
  dht.setup(DHTPIN, DHTesp::DHT22);

  // Display Init
  u8g2.begin();
  u8g2_prepare();
  u8g2.clearBuffer();

}

void bleSendValues(int value){
    String out = "BLE CO2: ";
    out += String(value);
    out += "ppm";
    ble.begin(out);
}

void drawToDisplay(int ppm, float temp, float humi){



  u8g2.clearBuffer();
  u8g2.setFont(STDFONT);
  u8g2.drawStr( 0, 0, "CO  Konzentration:");
  int coWidth = u8g2.getStrWidth("CO");
  u8g2.setFont(STDLOWFONT);
  u8g2.drawStr(coWidth + 1 , 3, "2");
  
  String displayPPMValue = String(ppm);
  
  // PPM Wert zeichnen
  u8g2.setFont(PPMFONT);
  int ppmWidth = u8g2.getStrWidth(displayPPMValue.c_str());
  
  // PPM Einheit zeichnen
  u8g2.setFont(PPMDEFFONT);
  int ppmTextWidth = u8g2.getStrWidth(PPMNAME);
  int combinedOffset = (u8g2.getDisplayWidth() - ppmWidth - ppmTextWidth - 2) / 2;

  u8g2.setFont(PPMFONT); 
  u8g2.drawStr( combinedOffset, PPMTOPOFFSET, displayPPMValue.c_str());

  u8g2.setFont(PPMDEFFONT);
  u8g2.drawStr( combinedOffset + ppmWidth + 2 , PPMTOPOFFSET + 15, PPMNAME);

  u8g2.setFont(TEMPFONT);
  String tempStr = "(";
  tempStr += String(temp, 1);
  tempStr += "°C";
  tempStr += "     ";
  tempStr += String(humi, 1);
  tempStr += "%)";

  int tempWidth = u8g2.getUTF8Width(tempStr.c_str());
  int tempLeftOff = (u8g2.getDisplayWidth() - tempWidth) / 2;
  
  u8g2.drawUTF8( tempLeftOff, TEMPTOPOFFSET, tempStr.c_str());

  u8g2.setFont(INFOFONT);
  //u8g2.drawStr( 30, 50, "Bitte lüften!");
  //u8g2.drawStr( 15, 56, "Gute Raumluft!");

  String infoText = "- - - - -";
  if(ppm < 400){
    infoText = "Datenfehler!";  
  } else if(ppm < 600){
    infoText = "Sehr gute Luft!";  
  } else if(ppm <= 800){
    infoText = "Gute Innenluft!";  
  } else if (ppm <= 1000){
    infoText = "Bald lüften!";  
  } else if (ppm <= 1400){
    infoText = "Schlechte Luft!";
  } else if (ppm <= 2000) {
    infoText = "Sehr schlechte Luft!";  
  } else {
    infoText = "Krass schlechte Luft!";  
  }

  int infoWidth = u8g2.getUTF8Width(infoText.c_str());
  int infoLeftOff = (u8g2.getDisplayWidth() - infoWidth) / 2;
  u8g2.drawUTF8(infoLeftOff, INFOTOPOFFSET, infoText.c_str());

  u8g2.sendBuffer();
}

void setLedInfo(int ppm){

  digitalWrite(LEDRED, LOW);
  digitalWrite(LEDYEL, LOW);
  digitalWrite(LEDGRN, LOW);
  digitalWrite(BUZZER, LOW); 

  if(ppm < 400){
    
  } else if(ppm < 600){
    digitalWrite(LEDGRN, HIGH); 
  } else if(ppm <= 800){
    digitalWrite(LEDGRN, HIGH);  
  } else if (ppm <= 1000){
    digitalWrite(LEDYEL, HIGH); 
  } else if (ppm <= 1400){
    digitalWrite(LEDRED, HIGH); 
  } else if (ppm <= 2000) {
    digitalWrite(LEDRED, HIGH); 
  } else {
    digitalWrite(LEDRED, HIGH); 
    digitalWrite(BUZZER, HIGH); 
  }  
}

void loop() {
  if (millis() - dataTimer >= 2500){
        int co2value = myMHZ19.getCO2();

        TempAndHumidity dhtValues = dht.getTempAndHumidity();
        if (dht.getStatus() != 0) {
          Serial.println("DHT22 error status: " + String(dht.getStatusString()));
        }
        
        Serial.print("CO2 (ppm): ");                      
        Serial.println(co2value);    

        Serial.print("Temp (°C): ");                      
        Serial.println(dhtValues.temperature);   

        Serial.print("Humidity (%): ");                      
        Serial.println(dhtValues.humidity);
                    

        bleSendValues(co2value);  
        setLedInfo(co2value);                           

        dataTimer = millis();

        drawToDisplay(co2value, dhtValues.temperature, dhtValues.humidity);

        if(co2value < 400){
          recalibrationTrigger++;  
        } else {
          recalibrationTrigger = 0;  
        }

        if(recalibrationTrigger > 24){
          recalibrationTrigger = 0; 
          myMHZ19.calibrateZero();

          u8g2.clearBuffer();
          u8g2.setFont(STDFONT);
          String infoText = "Rekalibrierung";
          int infoWidth = u8g2.getUTF8Width(infoText.c_str());
          int infoLeftOff = (u8g2.getDisplayWidth() - infoWidth) / 2;
          u8g2.drawUTF8(infoLeftOff, 30, infoText.c_str());
          u8g2.sendBuffer();
          delay(10000);
        }
    }
}
