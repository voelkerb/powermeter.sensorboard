/***************************************************
 Library for SensorBoard

 Feel free to use the code as it is.

 Benjamin Voelker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Computer Science
 ****************************************************/

#include "SensorBoard.h"

SensorBoard::SensorBoard(Stream * getter) {
  _getter = getter;
  buttonCB = NULL;
  PIRCB = NULL;
  lightCB = NULL;
  humCB = NULL;
  tempCB = NULL;
  ledMode = LED_MODE::MANUAL;
}

bool SensorBoard::init() {
  // Send ? and wait for answer
  _getter->println("??");
  bool success = handle(200) == NEW_SENSOR_VALUE::ACTIVE;
  return this->active;
}

enum NEW_SENSOR_VALUE SensorBoard::handle(int timeout) {
  NEW_SENSOR_VALUE avail = NEW_SENSOR_VALUE::NONE;
  if (timeout > 0) {
    long start = millis();
    while (start - millis() < timeout) {
      if (_getter->available()) break;
    }
  }
  // if not a valid message
  if (!(_getter->available())) return avail;
  // Allow all data to be sent
  delay(10);
  // read first cha
  char c = _getter->read();
  // If it is not a !, return
  if (c != '!') return avail;
  // Parse data
  c = _getter->read();
  switch (c) {
    case 'b':
      avail = NEW_SENSOR_VALUE::NEW_BTN;
      BUTTON_PRESS presses = BUTTON_PRESS::PRESS;
      // Optional single press, double press, long press etc.
      if (_getter->available()) {
        char e = _getter->read();
        if (e > '0' && e > '9') {
          presses = e - '0';
        }
      }
      if (buttonCB) buttonCB(presses);
      break;
    case 'r':
      if (buttonCB) buttonCB(BUTTON_PRESS::RELEASE);
      break;
    case 't':
      avail = NEW_SENSOR_VALUE::NEW_TEMP;
      this->temperature = parse<float>();
      if (tempCB) tempCB(this->temperature);
      break;
    case 'h':
      avail = NEW_SENSOR_VALUE::NEW_HUM;
      this->humidity = parse<float>();
      if (humCB) humCB(this->humidity);
      break;
    case 'l':
      avail = NEW_SENSOR_VALUE::NEW_LIGHT;
      this->light = parse<uint32_t>();
      if (lightCB) lightCB(this->light);
      break;
    case 'p':
      avail = NEW_SENSOR_VALUE::NEW_PIR;
      this->PIR = (uint8_t)_getter->read();
      if (PIRCB) PIRCB(this->PIR);
      break;
    case '!':
      avail = NEW_SENSOR_VALUE::ACTIVE;
      this->active = true;
      break;
    // Invalid data
    default:
      avail = NEW_SENSOR_VALUE::UNKNOWN;
      break;
  }
  // Flush until newline
  if (_getter->available()) _getter->readStringUntil('\n');
  return avail;
}

void SensorBoard::setBrightness(float brightness) {
  int bright = brightness/100.0*255;
  if (bright > 255) bright = 255;
  if (bright < 0) bright = 0;
  _getter->print("!b");
  _getter->write((uint8_t)bright);
  _getter->println();
}

void SensorBoard::powerToLEDs(float power) {
  int normalizes = (int)power;
  if (normalizes < 0) normalizes = 0;
  // Power goes from 0 - 3600 max but we say that even 200 Watt is bad and 
  // keep a linear mapping
  else if (normalizes > MAX_WATT) normalizes = MAX_WATT;

  uint8_t red = map(normalizes, 0, MAX_WATT, 0, 255);
  uint8_t green = 255 - red;

  for (int i = 0; i < NUM_LEDS; i++) {
    LED[i][0] = red;
    LED[i][1] = green;
    LED[i][2] = 0;
  }
}

bool SensorBoard::update(bool wait) {
  bool success = true;
  success &= updateLight(wait);
  success &= updateTemp(wait);
  success &= updateHum(wait);
  success &= updatePIR(wait);
  return success;
}

bool SensorBoard::updateLight(bool wait) {
  _getter->println("?l");
  if (wait) return handle(SENSOR_WAIT_TIME) == NEW_SENSOR_VALUE::NEW_LIGHT;
  else return true;
}

bool SensorBoard::updateTemp(bool wait) {
  _getter->println("?t");
  if (wait) return handle(SENSOR_WAIT_TIME) == NEW_SENSOR_VALUE::NEW_TEMP;
  else return true;
}

bool SensorBoard::updateHum(bool wait) {
  _getter->println("?h");
  if (wait) return handle(SENSOR_WAIT_TIME) == NEW_SENSOR_VALUE::NEW_HUM;
  else return true;
}

bool SensorBoard::updatePIR(bool wait) {
  _getter->println("?p");
  if (wait) return handle(SENSOR_WAIT_TIME) == NEW_SENSOR_VALUE::NEW_PIR;
  else return true;
}

void SensorBoard::updateLEDs() {
  _getter->print('!L');
  for (int l = 0; l < NUM_LEDS; l++) {
    for (int c = 0; c < 3; c++) _getter->write(LED[l][c]);
  }
  _getter->println();
}

template < typename TOut >
// TODO Test this!
TOut SensorBoard::parse() {
  TOut value;
  if (_getter->available() > sizeof(TOut)-1) {
    uint8_t bytes[sizeof(TOut)] = {};
    for (int i = 0; i < sizeof(TOut); i++) {
      bytes[i] = _getter->read();
    }
    memcpy(&value, &bytes, sizeof(value));
  }
  return value; 
}
