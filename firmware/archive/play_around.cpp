// NOTES
// SPI and I²C pins for D1 Mini
// https://steve.fi/hardware/d1-pins/
// SDA => D2
// SCL => D1

// SPI pins
// Clock (sck) => D5
// Data-In (mosi) => D7
// Chip-Select (cs) => D8
// Command/Data selection (a0)

// M-CLK => D5
// MISO => D6
// MOSI => D7
// SPI Bus SS (CS) => D8

#include <Arduino.h>
#include <U8g2lib.h>

// #include <iostream>
// #include <string>

U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,D1,D2); // medium ram, medium speed
// U8G2_SH1106_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0,D1,D2); // medium ram, medium speed
// U8X8_SH1106_128X64_NONAME_SW_I2C u8x8(D1,D2);

void setup()
{
  u8g2.begin(); // most ram, most speed
  // u8g2.begin(); // medium ram, medium speed
  // u8x8.begin(); // fast, text only
}

void loop()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(0,20,"Hello World!");
  u8g2.sendBuffer();

  // u8g2.firstPage();
  // do {
  //   u8g2.setFont(u8g2_font_ncenB14_tr);
  //   u8g2.drawStr(0,24, "Hello World");
  // } while ( u8g2.nextPage() );

  // fast, text only
  // u8x8.setFont(u8x8_font_chroma48medium8_r);
  // u8x8.drawString(0,1,"Hello World!");
}

    // String thisString = String(i);
    // char printChar[sizeof(thisString)];
    // thisString.toCharArray(printChar, sizeof(printChar));