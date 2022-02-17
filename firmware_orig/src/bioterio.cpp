// NOTES
// SDA => D2
// SCL => D1

// always needed
#include <Arduino.h>

// these libs are part of espressif8266 toolbox
#include <ESP8266WiFi.h>
#include "secrets.h"

// for OLED screen
#include <U8g2lib.h> 
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,D1,D2); // full buffer size
const uint16_t FRAME_RATE = 3; // frames per second
const uint16_t FRAME_TIME_MILLIS = 1000./FRAME_RATE; // time per frame in milis.
const uint8_t LINE_SPACING = 8; // we can print an new, non-overlapping line

// LED control -----------------------------------------------------------------
#include <FastLED.h> 
// nice to have, also included in fast led
#include "..\lib\every_n_timer\every_n_timer.h" // required for every N seconds thing

FASTLED_USING_NAMESPACE

// defin LED parameters
#define DATA_PIN D5
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS 10 
#define BRIGHTNESS 25
CRGB leds[NUM_LEDS]; // contains led info, this we set first, the we call led show

// hardware interfacing
#include <Wire.h>
#include <SPI.h>

// adafruit IO and sensors
#include "..\lib\Adafruit IO Arduino\src\AdafruitIO_WiFi.h"
#include <Adafruit_BME280.h>
#include <RunningMedian.h> // used for smoother data...

// init two presure / humid sensors we use
Adafruit_BME280 RightBME; 
Adafruit_BME280 LeftBME;  

const uint8_t RIGHT_ADDRESS = 0x77;
const uint8_t LEFT_ADDDRESS = 0x76;

// setup adafruit stuff
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PWD);
AdafruitIO_Feed *bigTeraHumid = io.feed("a_big_tera_humid");
AdafruitIO_Feed *smallTeraHumid = io.feed("a_small_tera_humid");
AdafruitIO_Feed *bigTeraTemp = io.feed("a_big_tera_temp");
AdafruitIO_Feed *smallTerraTemp = io.feed("a_small_tera_temp");
AdafruitIO_Feed *minHumidFeed = io.feed("a_min_humid");
AdafruitIO_Feed *maxHumidFeed = io.feed("a_max_humid");

const uint16_t AIO_UPDADTE_TIME = 20; // update AIO every n seconds


// setup time
#include <time.h>
const int8_t TIME_ZONE = -2; // central european time

uint8_t humidRunning = false;
float minRequiredOnHumid = 0;  // keep at least this much humidity
float minRequiredOffHumid = 0; // humid until we reach this value

// calculate running medians for humids and temps
// number of samples = frame rate (== sampling rate) * update interval 
// factor 2 for extra smoothing...
RunningMedian LhsHumid = RunningMedian(AIO_UPDADTE_TIME*FRAME_RATE*2);
RunningMedian LhsTemp  = RunningMedian(AIO_UPDADTE_TIME*FRAME_RATE*2);
RunningMedian RhsHumid = RunningMedian(AIO_UPDADTE_TIME*FRAME_RATE*2);
RunningMedian RhsTemp  = RunningMedian(AIO_UPDADTE_TIME*FRAME_RATE*2);
uint32_t lastMillies = 0;

// function declarations...
void print_values_serial(Adafruit_BME280 *bmeSensor);
void send_aio_values(Adafruit_BME280 *insideSensor, Adafruit_BME280 *outsideSensor);
void updateMinHumid(AdafruitIO_Data *data);
void updateMaxHumid(AdafruitIO_Data *data);
void setup_leds();
void set_led_status(uint8_t status);
void pulse_leds(uint8_t nPulses, uint8_t pulseSpeed);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void setup()
{
  uint8_t thisLine = 1;
  // setup screen, say hello
  u8g2.begin(); 
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_mr); 
  u8g2.setCursor(0,LINE_SPACING*thisLine++); // start at first line, then increment
  u8g2.print("bioterio 0.2");
  u8g2.setCursor(0,LINE_SPACING*8); // print signature in last line
  u8g2.print("Laser Hannes 2020");
  u8g2.sendBuffer();
  delay(250);

  Serial.begin(921600);
  while (!Serial)
    ; // wait for serial monitor to open

  // setting up leds ===========================================================
  u8g2.setCursor(0,LINE_SPACING*thisLine++); 
  u8g2.print("Setting up leds...");
  u8g2.sendBuffer();
  setup_leds();
  u8g2.print("done!");
  set_led_status(1); // working

  // setting up the sensors ====================================================
  u8g2.setCursor(0,LINE_SPACING*thisLine++); 
  u8g2.print("Sensor searching...");
  u8g2.sendBuffer();

  while (!RightBME.begin(RIGHT_ADDRESS, &Wire))
  {
    delay(2500);
  }
  while (!LeftBME.begin(LEFT_ADDDRESS, &Wire))
  {
    delay(2500);
  }
  u8g2.print("done!");

  u8g2.setCursor(0,LINE_SPACING*thisLine++); 
  u8g2.print("Sensor setup...");
  u8g2.sendBuffer();
  // see https://cdn-shop.adafruit.com/datasheets/BST-BME280_DS001-10.pdf 
  // for datasheet of sensor
  RightBME.setSampling(Adafruit_BME280::MODE_NORMAL,
                Adafruit_BME280::SAMPLING_X16,  // temperature
                Adafruit_BME280::SAMPLING_NONE, // pressure
                Adafruit_BME280::SAMPLING_X16,  // humidity
                Adafruit_BME280::FILTER_OFF,
                Adafruit_BME280::STANDBY_MS_0_5);

  LeftBME.setSampling(Adafruit_BME280::MODE_NORMAL,
                  Adafruit_BME280::SAMPLING_X16,  // temperature
                  Adafruit_BME280::SAMPLING_NONE, // pressure
                  Adafruit_BME280::SAMPLING_X16,  // humidity
                  Adafruit_BME280::FILTER_OFF,
                  Adafruit_BME280::STANDBY_MS_0_5);
  // suggested rate is 1Hz (1s)
  u8g2.print("done!");

  // connect to io.adafruit.com ================================================
  u8g2.setCursor(0,LINE_SPACING*thisLine++); 
  u8g2.print("Ada connecting");
  u8g2.sendBuffer();

  io.connect(); // WiFi.begin(WIFI_SSID, WIFI_PWD) called here!
  minHumidFeed->onMessage(updateMinHumid);
  maxHumidFeed->onMessage(updateMaxHumid);

  // wait for a connection
  while (io.status() < AIO_CONNECTED)
  {
    u8g2.print(".");
    u8g2.sendBuffer();
    delay(500);
  }
  // we are connected
  u8g2.setCursor(0,LINE_SPACING*thisLine++); 
  u8g2.print(io.statusText());
  u8g2.sendBuffer();
  send_aio_values(&RightBME, &LeftBME);
  // get latest minMax values for humidity from AIO 
  minHumidFeed->get();
  maxHumidFeed->get();

  // connect to time server
  u8g2.setCursor(0,LINE_SPACING*thisLine++); 
  u8g2.print("Waiting for time");
  configTime(TIME_ZONE*3600, 0 , "pool.ntp.org", "time.nist.gov"); // 
  while (!time(nullptr)) {
    u8g2.print(".");
    u8g2.sendBuffer();
    delay(500);
  }

  // setup my own IO pins
  pinMode(D0, OUTPUT); // relais for fan and humidifier
  digitalWrite(D0, 1); // relais is active low
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void loop()
{
  io.run(); // required for all sketches.

  EVERY_N_MILLISECONDS(FRAME_TIME_MILLIS) // update display
  {
    // float currentLhsHumid = LeftBME.readHumidity();
    // float currentLhsTemp = LeftBME.readTemperature();
    LhsHumid.add(LeftBME.readHumidity());
    LhsTemp.add(LeftBME.readTemperature());
    RhsHumid.add(RightBME.readHumidity());
    RhsTemp.add(RightBME.readTemperature());

    uint8_t thisLine = 1;
    u8g2.clearBuffer();
    u8g2.setCursor(0,LINE_SPACING*thisLine++); 
    u8g2.print("      lhs  rhs  min max");

    u8g2.setCursor(0,LINE_SPACING*thisLine++); 
    u8g2.print(" Temp ");
    u8g2.print(LhsTemp.getMedian(), 1);
    u8g2.print(" ");
    u8g2.print(RhsTemp.getMedian(), 1);
    u8g2.print(" ");

    u8g2.setCursor(0,LINE_SPACING*thisLine++); 
    u8g2.print("Humid ");
    u8g2.print(LhsHumid.getMedian(), 1);
    u8g2.print(" ");
    u8g2.print(RhsHumid.getMedian(), 1);
    u8g2.print(" ");
    u8g2.print(minRequiredOnHumid, 0);
    u8g2.print(" ");
    u8g2.print(minRequiredOffHumid, 0);

    u8g2.setCursor(0,LINE_SPACING*thisLine++); 
    uint32_t thisTime = millis();
    uint32_t timeDiff = thisTime - lastMillies;
    lastMillies = millis();
    u8g2.print(timeDiff);

    thisLine++; // skip one line
    u8g2.setCursor(0,LINE_SPACING*thisLine++); 
    if (humidRunning)
      u8g2.print("Humidifier is ON");
    else
      u8g2.print("Humidifier is OFF");

    time_t now = time(nullptr);
    u8g2.setCursor(0,LINE_SPACING*8); 
    u8g2.print(ctime(&now)); 
    u8g2.sendBuffer();
  }

  EVERY_N_SECONDS(20) // send AIO values and update AIO status
  {
    // TODO check for reasonable values before sending them...
    send_aio_values(&RightBME, &LeftBME);
  }

  EVERY_N_SECONDS(10) // control humidifier
  {
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
    }
    else if (humidRunning && humidReached)
    {
      humidRunning = false;
      digitalWrite(D0, 1);
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
    delay(100);
    FastLED.show();
  }
  delay(100);
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
