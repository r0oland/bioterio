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

// these libs are part of espressif8266 toolbox
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>

// since latest version (28.04.2020) code no longer compiles...this libary version
// seems to be working though...
#include "..\lib\Adafruit IO Arduino\src\AdafruitIO_WiFi.h"
#include "..\lib\every_n_timer\every_n_timer.h" // required for every N seconds thing
#include <Adafruit_BME280.h>
// #include <FastLED.h> 

#include <U8g2lib.h> // for OLED screen

#include "secrets.h"

// function declarations...
void print_values_serial(Adafruit_BME280 *bmeSensor);
void send_aio_values(Adafruit_BME280 *insideSensor, Adafruit_BME280 *outsideSensor);
void updateMinHumid(AdafruitIO_Data *data);
void updateMaxHumid(AdafruitIO_Data *data);

U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,D1,D2); // full buffer size

// init two presure / humid sensors we use
Adafruit_BME280 RightBME; 
Adafruit_BME280 LeftBME;  

const uint8_t RIGHT_ADDRESS = 0x77;
const uint8_t LEFT_ADDDRESS = 0x76;

// setup adafruit stuff
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PWD);
AdafruitIO_Feed *bigTeraHumid = io.feed("b_big_tera_humid");
AdafruitIO_Feed *smallTeraHumid = io.feed("b_small_tera_humid");
AdafruitIO_Feed *bigTeraTemp = io.feed("b_big_tera_temp");
AdafruitIO_Feed *smallTerraTemp = io.feed("b_small_tera_temp");
AdafruitIO_Feed *minHumidFeed = io.feed("b_min_humid");
AdafruitIO_Feed *maxHumidFeed = io.feed("b_max_humid");

uint8_t humidRunning = false;
float minRequiredOnHumid = 0;  // keep at least this much humidity
float minRequiredOffHumid = 0; // humid until we reach this value

void setup()
{
  // setup screen, say hello
  u8g2.begin(); 
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_artosserif8_8r);
  u8g2.drawStr(0,20,"Hello World!");
  u8g2.sendBuffer();
  delay(1000);

  Serial.begin(921600);
  while (!Serial)
    ; // wait for serial monitor to open
  Serial.println("");
  Serial.println("");

  // setting up the sensors ====================================================
  u8g2.clearBuffer();
  u8g2.drawStr(0,20,"Sensor searching...");
  u8g2.sendBuffer();

  Serial.print("Looking for right humid. sensor...");
  while (!RightBME.begin(RIGHT_ADDRESS, &Wire))
  {
    Serial.println("Could not find right sensor, check wiring!");
    delay(2500);
  }
  Serial.println("found!");

  Serial.print("Looking for left humid. sensor...");
  while (!LeftBME.begin(LEFT_ADDDRESS, &Wire))
  {
    Serial.println("Could not find left sensor, check wiring!");
    delay(2500);
  }
  Serial.println("found!");

  // connect to io.adafruit.com
  u8g2.clearBuffer();
  u8g2.drawStr(0,20,"Ada connecting...");
  u8g2.sendBuffer();

  Serial.print("Connecting to Adafruit IO");
  io.connect();
  minHumidFeed->onMessage(updateMinHumid);
  maxHumidFeed->onMessage(updateMaxHumid);

  // wait for a connection
  while (io.status() < AIO_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  // we are connected
  Serial.println();
  Serial.println(io.statusText());
  send_aio_values(&RightBME, &LeftBME);
  minHumidFeed->get();
  maxHumidFeed->get();

  // setup my own IO pins
  pinMode(D0, OUTPUT); // relais for fan and humidifier
  digitalWrite(D0, 1); // relais is active low
  pinMode(D5, INPUT); // button pin

  u8g2.begin();
}

void loop()
{
  io.run(); // required for all sketches.

  // picture loop
  // u8g.firstPage();  
  // do {
  //   draw();
  // } while( u8g.nextPage() );
  

  EVERY_N_SECONDS(5) // print serial values...
  {
    Serial.println("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");
    Serial.println("Right Terarium:");
    RightBME.takeForcedMeasurement();
    print_values_serial(&RightBME);

    Serial.println("Left Terarium:");
    LeftBME.takeForcedMeasurement();
    print_values_serial(&LeftBME);

    Serial.print("Humid running: ");
    Serial.println(humidRunning);
  }

  EVERY_N_SECONDS(20) // send AIO values and update AIO status
  {
    // TODO check for reasonable values before sending them...
    send_aio_values(&RightBME, &LeftBME);
  }

  EVERY_N_SECONDS(10)
  {
    // get latest humidity reading
    RightBME.takeForcedMeasurement();
    LeftBME.takeForcedMeasurement();
    float rightHumid = RightBME.readHumidity();
    float leftHumid = LeftBME.readHumidity();

    // if one is to low, turn on humidifier
    bool leftHumidToLow = (leftHumid < minRequiredOnHumid);
    bool rightHumidToLow = (rightHumid < minRequiredOnHumid);
    bool humidToLow = rightHumidToLow || leftHumidToLow;

    // if both are high enough, turn off humidifier
    bool leftHumidReached = (leftHumid > minRequiredOffHumid);
    bool rightHumidReached = (rightHumid > minRequiredOffHumid);
    bool humidReached = leftHumidReached && rightHumidReached;
    if (humidToLow && !humidRunning)
    {
      humidRunning = true;
      digitalWrite(D0, 0);
      Serial.println("Turned ON humidifier");
    }
    else if (humidRunning && humidReached)
    {
      humidRunning = false;
      digitalWrite(D0, 1);
      Serial.println("Turned OFF humidifier");
    }
  }
}

// void draw() {
//   // u8g.setFont(u8g_font_unifont);
//   // u8g.drawStr(0, 1, "H!");
// }

void print_values_serial(Adafruit_BME280 *bmeSensor)
{
  Serial.println("-------------------------------------------");
  Serial.print("Temperature = ");
  Serial.print(bmeSensor->readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bmeSensor->readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Humidity = ");
  Serial.print(bmeSensor->readHumidity());
  Serial.println(" %");

  Serial.println("-------------------------------------------");
}

void send_aio_values(Adafruit_BME280 *insideSensor, Adafruit_BME280 *outsideSensor)
{
  bigTeraHumid->save(insideSensor->readHumidity());
  bigTeraTemp->save(insideSensor->readTemperature());

  smallTeraHumid->save(outsideSensor->readHumidity());
  smallTerraTemp->save(outsideSensor->readTemperature());
}

void updateMinHumid(AdafruitIO_Data *data)
{
  minRequiredOnHumid = data->toFloat();
  Serial.print("New minimum Humidity: ");
  Serial.println(minRequiredOnHumid);
}

void updateMaxHumid(AdafruitIO_Data *data)
{
  minRequiredOffHumid = data->toFloat();
  Serial.print("New maximum Humidity: ");
  Serial.println(minRequiredOffHumid);
}
