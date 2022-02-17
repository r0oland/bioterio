#include <Arduino.h>

#include <U8g2lib.h> 
#include <Wire.h>

uint16_t line = 0;
char snum[5];



U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

void setup(void) {
  u8g2.begin();
  u8g2.enableUTF8Print();
}

uint32_t i = 0;

void loop(void) {
  // u8g2.firstPage();
  // do {
  //   u8g2.setFont(u8g2_font_ncenB14_tr);
  //   u8g2.drawStr(0,24,"Hello World!");
  // } while ( u8g2.nextPage() );
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.setCursor(0, 20);
  u8g2.print(i++);
  // u8g2.drawStr(0,20,"Hello World!");
  u8g2.setCursor(20, 20);
  u8g2.print(++i, HEX);

  snprintf(snum, 5, "%d", i);

  u8g2.drawStr(0, 40, snum);
  u8g2.print(" ");
  u8g2.print((float)i/22.33,2);

  u8g2.setFont(u8g2_font_unifont_t_symbols);
  u8g2.drawGlyph(0, 64, 0x2603);	/* dec 9731/hex 2603 Snowman */

  u8g2.setFont(u8g2_font_streamline_pet_animals_t);
  u8g2.drawGlyph(32, 64, 0x33);	

  u8g2.setFont(u8g2_font_streamline_internet_network_t);
  u8g2.drawGlyph(64, 64, 0x31);	
  if (++i % 2) {
    u8g2.drawGlyph(64, 64, 0x31);	
  } else {
    u8g2.drawGlyph(64, 64, 0x38);	
  }

  u8g2.sendBuffer();

  delay(100);
}