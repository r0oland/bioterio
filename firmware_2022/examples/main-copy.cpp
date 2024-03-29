#include <Arduino.h>

#include "Wire.h"
#include "SHT2x.h"

uint32_t start;
uint32_t stop;

SHT2x sht;
uint32_t connectionFails = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("SHT2x_LIB_VERSION: \t");
  Serial.println(SHT2x_LIB_VERSION);

  sht.begin();

  uint8_t stat = sht.getStatus();
  Serial.print(stat, HEX);
  Serial.println();
}


void loop()
{
  if ( sht.isConnected()  )
  {
    start = micros();
    bool b = sht.read();
    stop = micros();

    int error = sht.getError();
    uint8_t stat = sht.getStatus();

    Serial.print(millis());
    Serial.print("\t");
    Serial.print(stop - start);
    Serial.print("\t");
    Serial.print(b, HEX);
    Serial.print("\t");
    Serial.print(error, HEX);
    Serial.print("\t");
    Serial.print(stat, HEX);
    Serial.print("\t");
    Serial.print(sht.getTemperature(), 1);
    Serial.print("\t");
    Serial.print(sht.getHumidity(), 1);
  }
  else
  {
    connectionFails++;
    Serial.print(millis());
    Serial.print("\tNot connected:\t");
    Serial.print(connectionFails);
    // sht.reset();
  }
  Serial.println();
  delay(1000);
}
