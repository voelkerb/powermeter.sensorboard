// Arduino ATMEGA328 8MHz Sketch with Sensors: DHT 11, Grove Button, LED WS2812b Light Sensor and PIR Sensor

// #include <ArduinoJson.h>
// #include <stdio.h>
// #include <string>

// sensor libraries
#include <FastLED.h>
#include <DHT.h>
#include <Wire.h>
#include <Digital_Light_TSL2561.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// Choose one of the following
// Either you get press and release 
// #define STATEFULL_PRESS
// Or you get Single press, double press, long press etc.


#if defined(ESP8266)
void ICACHE_RAM_ATTR buttonISR();
#endif
enum class BUTTON_PRESS {NONE=0, SINGLE=1, DOUBLE=2, LONG_START=3, RELEASE=4};
void buttonPressed(BUTTON_PRESS press);
// pins
#define LED_PIN 5
#define PIR_PIN 7
#define DHT_PIN 9
// TODO
#if defined(ESP8266)
#define BUTTON_PIN D6
#else 
#define BUTTON_PIN 3
#endif

#define BRIGHTNESS_ADDRESS 0

// Serial stuff
#define SERIAL_SPEED 38400
// #define DEBUG
#define USE_SENSORS
#define debugSerial Serial
#define com Serial

// LED
// Define the array of leds
#define NUM_LEDS 3
CRGB leds[NUM_LEDS];
CRGB fadeColor[NUM_LEDS];
CRGB black = CRGB::Black;
bool ledUpdate = false;

//DHT
#ifdef USE_SENSORS
DHT dht(DHT_PIN, DHT22);
#endif
// automaticall send data on change and with hysteresis
bool autoSend = false;

// old values for sensors
float temp = -1;
float hum = -1;
int light = -1;
uint8_t pir = 255;

uint8_t brightness = 255;

// Update timers
long lightUpdate = millis();
long tempUpdate = millis();
long humUpdate = millis();
#define LIGHT_UPDATE_INTV 5000
#define TEMP_UPDATE_INTV 5000
#define HUM_UPDATE_INTV TEMP_UPDATE_INTV // as it is from the same sensor

uint8_t fadeDelay = 5;

#define DOUBLE_PRESS_DELAY 900
#define LONG_PRESS_DELAY 5000
// Button pressed flag handled in loop
bool buttonReleased = false;
volatile int pressed = 0;
volatile long lastPressed = millis();

volatile bool oldState = false;
volatile long lastTime = millis();

#ifdef STATEFULL_PRESS
void handleButton() {
  if (pressed != 0) {
    BUTTON_PRESS buttonPress = BUTTON_PRESS::NONE;
    // Button was already long pressed and is now released
    if (pressed == -1 and buttonReleased) {
      buttonPress = BUTTON_PRESS::RELEASE;
      pressed = 0;
    // Button has been pressed
    } else if (pressed > 0) {
      // Button has been released and doublepress detection time has passed
      if (buttonReleased and millis()-lastPressed > DOUBLE_PRESS_DELAY) {
        // number stored in pressed corresponds to the number of presses
        if (pressed == 1) {
          buttonPress = BUTTON_PRESS::SINGLE;
        } else if (pressed == 2) {
          buttonPress = BUTTON_PRESS::DOUBLE;
        } else {
          // Treet as single press
          buttonPress = BUTTON_PRESS::SINGLE;
        }
        // Reset variable
        pressed = 0;
      // Button not yet released but long press delay already passed
      } else if (!buttonReleased and millis()-lastPressed > LONG_PRESS_DELAY) {
        // If long press start not indicated yet
        if (pressed != -1) {
          buttonPress = BUTTON_PRESS::LONG_START;
          pressed = -1;
        } 
      }  
    }
    if (buttonPress != BUTTON_PRESS::NONE) buttonPressed(buttonPress);
  }
}

void buttonPressed(BUTTON_PRESS press) {
  switch (press) {
    case BUTTON_PRESS::NONE:
      break;
    case BUTTON_PRESS::SINGLE:
      #ifdef DEBUG
      debugSerial.println("Single press");
      #endif
      com.println("!b1");
      break;
    case BUTTON_PRESS::DOUBLE:
      #ifdef DEBUG  
      debugSerial.println("Double press");
      #endif
      com.println("!b2");
      break;
    case BUTTON_PRESS::LONG_START:
      #ifdef DEBUG
      debugSerial.println("Long press start");
      #endif
      com.println("!b3");
      break;
    case BUTTON_PRESS::RELEASE:
      #ifdef DEBUG
      debugSerial.println("Long press end");
      #endif
      com.println("!b4");
      break;
    default:
      break;
  }
}

#if defined(ESP8266)
void ICACHE_RAM_ATTR buttonISR() {
#else
void buttonISR() {
#endif
  bool state = !digitalRead(BUTTON_PIN);
  if (state == oldState) return;
  // Debounce delay
  if (millis()-lastTime < 10) return;
  lastTime = millis();
  if (state) {
    #ifdef DEBUG
    debugSerial.println("Pressed");
    #endif
    pressed++;
    lastPressed = millis();
    buttonReleased = false;
  } else {
    #ifdef DEBUG
    debugSerial.println("Released");
    #endif
    buttonReleased = true;
  }
  oldState = state;
}

#else 
#if defined(ESP8266)
void ICACHE_RAM_ATTR buttonISR() {
#else
void buttonISR() {
#endif
  bool state = !digitalRead(BUTTON_PIN);
  if (state == oldState) return;
  // Debounce delay
  if (millis()-lastTime < 10) return;
  lastTime = millis();
  if (state) {
    #ifdef DEBUG
    debugSerial.println("Pressed");
    #endif
    com.println("!b");
  } else {
    #ifdef DEBUG
    debugSerial.println("Released");
    #endif
    com.println("!r");
  }
  oldState = state;
}
#endif

void setup() {
  #ifdef DEBUG
  #if debugSerial != com
  debugSerial.begin(SERIAL_SPEED);
  #endif
  #endif
  // Init serial
  com.begin(SERIAL_SPEED);

  #ifdef DEBUG
  debugSerial.println("Setup init");
  #endif

  // Init LEDs
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS); 
  // Tests LEDs
  setAllLEDs(CRGB::Red);
  delay(50);
  setAllLEDs(CRGB::Green);
  delay(50);
  setAllLEDs(CRGB::Blue);
  delay(50);
  allLEDs(fadeColor, NUM_LEDS, black);
  // Fade black
  ledUpdate = true;
  #if defined(ESP8266)
  EEPROM.begin(512);
  #endif
  // Load and set brightness
  EEPROM.get(BRIGHTNESS_ADDRESS, brightness);
  FastLED.setBrightness(brightness);
  // Set button as input and configure pin interrupt
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE); 
  #ifdef USE_SENSORS
  // PIR has digital input signal
  #ifdef DEBUG
  debugSerial.println("PIR init");
  pinMode(PIR_PIN, INPUT_PULLUP);
  #endif
  // Wire required for TSL
  Wire.begin();
  #ifdef DEBUG
  debugSerial.println("DHT init");
  #endif
  dht.begin();
  #ifdef DEBUG
  debugSerial.println("TSL2561 init");
  #endif
  TSL2561.init();
  #endif

  #ifdef DEBUG
  debugSerial.println("Setup done!");
  #endif
}



void loop() {
  #ifdef STATEFULL_PRESS
  handleButton();
  #endif
  #ifdef USE_SENSORS
  if (autoSend) {
    sendPIR(true);
    if (millis()-lightUpdate > LIGHT_UPDATE_INTV) {
      lightUpdate = millis();
      sendLight(true);
    }
    if (millis()-humUpdate > HUM_UPDATE_INTV) {
      humUpdate = millis();
      sendHum(true);
    }
    if (millis()-tempUpdate > TEMP_UPDATE_INTV) {
      tempUpdate = millis();
      sendTemp(true);
    }
  }
  #endif
  // As long as fade is not finished, continue fading
  if (ledUpdate) {
    ledUpdate = !fadeit();
    FastLED.show();
    // Becomes slightly irresponsive on LED updates
    FastLED.delay(fadeDelay); 
  }
  // recieve command from powermeter
  if (com.available()) handleEvent();
}


void handleEvent() {
  if (!(com.available()>2)) return;
  char c = com.read();
  // If it is not a !, return
  if (c != '?' and c != '!') return;
  // Let all data be sent by the sink
  delay(10);
  char d = com.read();
  switch (d) {
    case '?':
      com.println("!!");
      break;
    case 'b':
      brightness = com.read();
      EEPROM.put(BRIGHTNESS_ADDRESS, brightness);
      FastLED.setBrightness(brightness);
      FastLED.show();
      #ifdef DEBUG
      debugSerial.print("Set brightess to: ");
      EEPROM.get(BRIGHTNESS_ADDRESS, brightness);
      debugSerial.println(brightness);
      #endif
      break;
    case 'a':
      autoSend = true;
      //  New values on next run
      lightUpdate = millis()-LIGHT_UPDATE_INTV;
      humUpdate = millis()-HUM_UPDATE_INTV;
      tempUpdate = millis()-TEMP_UPDATE_INTV;
      temp = -1;
      hum = -1;
      light = -1;
      pir = 255;
      #ifdef DEBUG
      debugSerial.println("Autosend on");
      #endif
      break;
    case 'o':
      autoSend = false;
      #ifdef DEBUG
      debugSerial.println("Autosend off");
      #endif
      break;
    #ifdef USE_SENSORS
    case 'p':
      sendPIR(false);
      break;
    case 't':
      sendTemp(false);
      break;
    case 'h':
      sendHum(false);
      break;
    case 'l':
      sendHum(false);
      break;
    #endif
    case 'L':
      delay(10);
      if (com.available() < 3*NUM_LEDS) return;
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t r = com.read();
        uint8_t g = com.read();
        uint8_t b = com.read();
        fadeColor[i] = CRGB(r,g,b);
      }
      ledUpdate = true;
      #ifdef DEBUG
      debugSerial.print("Set LED to: ");
      for (int i = 0; i < NUM_LEDS; i++) {
        debugSerial.print("[");
        debugSerial.print(fadeColor[i].red);
        debugSerial.print(",");
        debugSerial.print(fadeColor[i].green);
        debugSerial.print(",");
        debugSerial.print(fadeColor[i].blue);
        debugSerial.print("] ");
      }
      debugSerial.println();
      #endif
      break;
    default:
      #ifdef DEBUG
      debugSerial.println("Unknown command");
      #endif
      break;
  }
}

#ifdef USE_SENSORS
void sendPIR(bool onNew) {
  uint8_t newPIR = (uint8_t)digitalRead(PIR_PIN);
  if (onNew and newPIR == pir) return;
  pir = newPIR;
  com.print("!p");
  com.write(pir);
  com.println();
}

void sendTemp(bool onNew) {
  float newTemp = (float)dht.readTemperature();
  //  Measurement error
  if (newTemp == -1) {
    if (!onNew) newTemp = temp;
    else return;
  }
  if (onNew and newTemp == temp) return;
  temp = newTemp;
  byte * b = (byte *) &temp;
  com.print("!t");
  com.write(b,4);
  com.println();
}

void sendHum(bool onNew) {
  float newHum = (float)dht.readHumidity();
  //  Measurement error
  if (newHum == -1) {
    if (!onNew) newHum = hum;
    else return;
  }
  if (onNew and newHum == hum) return;
  hum = newHum;
  byte * b = (byte *) &hum;
  com.print("!h");
  com.write(b,4);
  com.println();
}

void sendLight(bool onNew) {
  uint32_t newLight = (uint32_t)TSL2561.readVisibleLux();
  if (onNew and newLight == light) return;
  light = newLight;
  byte * b = (byte *) &light;
  com.print("!l");
  com.write(b,4);
  com.println();
}
#endif

void allLEDs(CRGB *colors, uint32_t N, CRGB color) {
  for (int i = 0; i < N; i++) colors[i] = color;
}

void setAllLEDs(CRGB color) {
  allLEDs(leds, NUM_LEDS, color);
  FastLED.show();
}

bool fadeit() {
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    fadeTowardColor(leds[i], fadeColor[i], 10);
  }
  // Check if now done
  bool allDone = true;
  for (int i = 0; i < NUM_LEDS; i++) {
    if (leds[i] != fadeColor[i]) {
      allDone = false;
      break;
    }
  }
  return allDone;
}


// Helper function that blends one uint8_t toward another by a given amount
void nblendU8TowardU8(uint8_t& cur, const uint8_t target, uint8_t amount) {
  if (cur == target) return;
  if (cur < target) {
    uint8_t delta = target - cur;
    delta = scale8_video( delta, amount);
    cur += delta;
  } else {
    uint8_t delta = cur - target;
    delta = scale8_video( delta, amount);
    cur -= delta;
  }
}

// Blend one CRGB color toward another CRGB color by a given amount.
// Blending is linear, and done in the RGB color space.
// This function modifies 'cur' in place.
CRGB fadeTowardColor(CRGB& cur, const CRGB& target, uint8_t amount) {
  nblendU8TowardU8(cur.red,   target.red,   amount);
  nblendU8TowardU8(cur.green, target.green, amount);
  nblendU8TowardU8(cur.blue,  target.blue,  amount);
  return cur;
}
