#include <Arduino.h>

#include <U8g2lib.h> 
#include <Wire.h>

uint16_t line = 0;
char lineBuffer[20];



U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

void setup(void) {
  u8g2.begin();
  u8g2.enableUTF8Print();
}

uint32_t i = 0;

void loop(void) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_smart_patrol_nbp_tf);

  snprintf(lineBuffer, 20, "Humid: %03.2f %%", 22.33);
  u8g2.drawStr(0, 20, lineBuffer);

  snprintf(lineBuffer, 20, " Temp: %03.2f %%", 22.33);
  u8g2.drawStr(0, 40, lineBuffer);

  u8g2.setFont(u8g2_font_streamline_internet_network_t);
  u8g2.setFontMode(1);  /* activate transparent font mode */
  u8g2.setDrawColor(0);
  u8g2.drawGlyph(100, 64, 0x31);	// WIFI not working
  delay(500);
  u8g2.sendBuffer();

  u8g2.setDrawColor(2);
  u8g2.drawGlyph(100, 64, 0x31);	// WIFI not working
  delay(500);
  u8g2.sendBuffer();




  // u8g2.drawGlyph(64, 64, 0x38);	// WIFI working 
}