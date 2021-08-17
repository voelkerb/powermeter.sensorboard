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

// pins
#define LED_PIN 5
#define PIR_PIN 7
#define DHT_PIN 9
#define BUTTON_PIN 3

#define BRIGHTNESS_ADDRESS 0

// Serial stuff
#define SERIAL_SPEED 38400
//# define DEBUG
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
DHT dht(DHT_PIN, DHT22);

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

uint8_t fadeDelay = 10;

void setup() {
  #ifdef DEBUG
  debugSerial.begin(SERIAL_SPEED);
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
  delay(200);
  setAllLEDs(CRGB::Green);
  delay(200);
  setAllLEDs(CRGB::Blue);
  delay(200);
  allLEDs(fadeColor, NUM_LEDS, black);
  // Fade black
  ledUpdate = true;
  // Load and set brightness
  EEPROM.get(BRIGHTNESS_ADDRESS, brightness);
  FastLED.setBrightness(brightness);

  // Set button as input and configure pin interrupt
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING); 

  // PIR has digital input signal
  pinMode(PIR_PIN, INPUT);

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
  #ifdef DEBUG
  debugSerial.println("Setup done!");
  #endif
}

// Debounce timer
long buttonDebounce = millis();
// Flag handled in loop
bool buttonPressed = false;
void buttonISR() {
  if (millis()-buttonDebounce > 300) {
    // buttonPressed = true;
    buttonDebounce = millis();
    // // If serial communication was interrupted
    // com.flush();
    // delay(1);
    // com.println("!b");
    // delay(1);
    buttonPressed = true;
  }
}

void loop() {
  // NOTE: Sensor reading might take much time
  // if (buttonPressed) {
  //   #ifdef DEBUG
  //   debugSerial.println("Button Pressed");
  //   #endif
  //   com.println("!b");
  //   buttonPressed = false;
  // }

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
  if (!com.available()) return;
  char c = com.read();
  // If it is not a !, return
  if (c != '?' and c != '!') return avail;
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
    case 'o':
      autoSend = false;
      break;
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
      break;
    default:
      #ifdef DEBUG
      debugSerial.println("Unknown command");
      #endif
      break;
  }
}

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

void allLEDs(CRGB *colors, uint32_t N, CRGB color) {
  for (int i = 0; i < N; i++) colors[i] = color;
}

void setAllLEDs(CRGB color) {
  allLEDs(leds, NUM_LEDS, color);
  FastLED.show();
}



bool fadeit() {
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    fadeTowardColor(leds[i], fadeColor[i], 1);
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
