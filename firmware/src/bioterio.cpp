// NOTES
// SDA => D2
// SCL => D1

#include <Arduino.h>

// these libs are part of espressif8266 toolbox
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>

#include "..\lib\Adafruit IO Arduino\src\AdafruitIO_WiFi.h"
#include <Adafruit_BME280.h>

#include <FastLED.h> 
#include "..\lib\every_n_timer\every_n_timer.h" // required for every N seconds thing

FASTLED_USING_NAMESPACE
// defin LED parameters
#define DATA_PIN D5
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS 10 
#define BRIGHTNESS 25
#define FRAMES_PER_SECOND 30
CRGB leds[NUM_LEDS]; // contains led info, this we set first, the we call led show

#include <U8g2lib.h> // for OLED screen

#include "secrets.h"

// function declarations...
void print_values_serial(Adafruit_BME280 *bmeSensor);
void send_aio_values(Adafruit_BME280 *insideSensor, Adafruit_BME280 *outsideSensor);
void updateMinHumid(AdafruitIO_Data *data);
void updateMaxHumid(AdafruitIO_Data *data);
void setup_leds();
void set_led_status(uint8_t status);
void pulse_leds(uint8_t nPulses, uint8_t pulseSpeed);

U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,D1,D2); // full buffer size
const uint16_t FRAME_RATE = 1000; // frames per second
const uint16_t FRAME_TIME_MILLIS = 1000./FRAME_RATE; // time per frame in milis.

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
  // u8g2.setFont(u8g2_font_artosserif8_8r);
  u8g2.setFont(u8g2_font_5x8_mr ); 
  u8g2.drawStr(0,8,"bioterio");
  u8g2.sendBuffer();
  delay(1000);

  Serial.begin(921600);
  while (!Serial)
    ; // wait for serial monitor to open
  Serial.println("");
  Serial.println("");

  Serial.print("Setting up leds...");
  setup_leds();
  Serial.println("done!");
  set_led_status(1); // working

  // setting up the sensors ====================================================
  u8g2.drawStr(0,16,"Sensor searching...");
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
  u8g2.setCursor(0,24);
  u8g2.print("Ada connecting");
  u8g2.sendBuffer();

  Serial.print("Connecting to Adafruit IO");
  io.connect();
  minHumidFeed->onMessage(updateMinHumid);
  maxHumidFeed->onMessage(updateMaxHumid);

  // wait for a connection
  while (io.status() < AIO_CONNECTED)
  {
    u8g2.print(".");
    u8g2.sendBuffer();
    Serial.print(".");
    delay(250);
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
  // pinMode(D5, INPUT); // button pin

}

void loop()
{
  io.run(); // required for all sketches.

  EVERY_N_MILLISECONDS(FRAME_TIME_MILLIS) // print serial values...
  {
    RightBME.takeForcedMeasurement();
    LeftBME.takeForcedMeasurement();      

    u8g2.clearBuffer();
    u8g2.setCursor(0,8);
    u8g2.print("      Left Right Min");
    u8g2.setCursor(0,16);
    u8g2.print(" Temp ");
    u8g2.print(LeftBME.readTemperature(), 1);
    u8g2.print(" ");
    u8g2.print(RightBME.readTemperature(), 1);
    u8g2.print(" ");

    u8g2.setCursor(0,24);
    u8g2.print("Humid ");
    u8g2.print(LeftBME.readHumidity(), 1);
    u8g2.print(" ");
    u8g2.print(RightBME.readHumidity(), 1);
    u8g2.print(" ");

    u8g2.sendBuffer();
  }

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

void setup_leds()
{
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  FastLED.clear();
  FastLED.show();
  for (int led = 0; led < NUM_LEDS; led++)
  {
    leds[led] = CRGB::White;
    delay(200);
    FastLED.show();
  }
  delay(200);
  pulse_leds(3, 5);
  FastLED.clear();
  FastLED.show();
}

void pulse_leds(uint8_t nPulses, uint8_t pulseSpeed)
{
  uint8_t ledFade = 255;      // start with LEDs off
  int8_t additionalFade = -5; // start with LEDs getting brighter
  uint8_t iPulse = 0;

  while (iPulse < nPulses)
  {
    for (uint8_t iLed = 0; iLed < NUM_LEDS; iLed++)
    {
      leds[iLed].setRGB(255, 255, 255);
      leds[iLed].fadeLightBy(ledFade);
    }
    FastLED.show();
    ledFade = ledFade + additionalFade;
    // reverse the direction of the fading at the ends of the fade:
    if (ledFade == 0 || ledFade == 255)
      additionalFade = -additionalFade;
    if (ledFade == 255)
      iPulse++;
    delay(pulseSpeed); // This delay sets speed of the fade. I usually do from 5-75 but you can always go higher.
  }
}

void set_led_status(uint8_t status)
{
  // first led in array displays overall status
  // (0 = all good, 1 = working, 2 = error)
  switch (status)
  {
  case 0:
    leds[0].setRGB(0, 255, 0); // all good = green
    FastLED.show();
    break;
  case 1:
    leds[0].setRGB(200, 165, 0); // working == orange
    FastLED.show();
    break;
  case 2:
    leds[0].setRGB(255, 0, 0); // error == red
    FastLED.show();
    break;

  default:
    leds[0].setRGB(255, 0, 0);
    FastLED.show();
    break;
  }
}
