#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "secrets.h"

const uint8_t TIME_ZONE = -2; // central european time
int dst = 0;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  Serial.println("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  configTime(TIME_ZONE*-3600, 0 , "pool.ntp.org", "time.nist.gov"); // 
  Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
}

void loop() {
  time_t now = time(nullptr);
  Serial.println(ctime(&now));
  delay(1000);
}