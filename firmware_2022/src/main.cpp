#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <U8g2lib.h>

#include "SHT2x.h"
#include "secrets.h"

char lineBuffer[20];

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
SHT2x sht;

void setup(void) {

  Serial.begin(115200);

  Serial.print("u8g2\n");
  u8g2.begin();
  u8g2.enableUTF8Print();

  Serial.print("sht\n");
  sht.begin();
  sht.getStatus();

  // u8g2.setFont(u8g2_font_streamline_internet_network_t);
  while (!sht.isConnected()) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\n");
}

float temp = 0.00;
float humid = 0.00;

void loop(void) {

  if (sht.isConnected()) {
    sht.read();
    sht.getError();
    sht.getStatus();
    humid = sht.getHumidity();
    temp = sht.getTemperature();
    Serial.print("humid ");
    Serial.print(humid);
    Serial.print(" temp ");
    Serial.print(temp);
    Serial.print("\n");
  } else {
    temp = -99.9;
    humid = -99.9;
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_smart_patrol_nbp_tf);
  
  snprintf(lineBuffer, 20, "Humid: %03.2f %%", humid);
  u8g2.drawStr(0, 20, lineBuffer);

  snprintf(lineBuffer, 20, " Temp: %03.2f %%", temp);
  u8g2.drawStr(0, 40, lineBuffer);

  u8g2.setFont(u8g2_font_streamline_internet_network_t);
  u8g2.drawGlyph(100, 64, 0x31);  // WIFI not working
  // u8g2.setFontMode(1); /* activate transparent font mode */
  // u8g2.setDrawColor(0);
  
  u8g2.sendBuffer();


  delay(1000);
}