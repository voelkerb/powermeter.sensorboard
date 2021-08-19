/***************************************************
 Library for SensorBoard

 Feel free to use the code as it is.

 Benjamin Voelker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Computer Science
 ****************************************************/

#include "SensorBoard.h"

SensorBoard::SensorBoard(Stream * getter):
  patternUpdateTimes{0, BLINK_STEP, ROUND_STEP, GLOW_STEP, ACTIVE_POWER_UPDATE},
  updatePattern{&updateStaticPattern, &updateBlinkPattern, &updateRoundPattern, &updateGlowPattern, &updateActivePowerPattern} {
  _getter = getter;
  buttonCB = NULL;
  PIRCB = NULL;
  lightCB = NULL;
  humCB = NULL;
  tempCB = NULL;

  // Current pattern
  mainColor = CRGB{255,0,0};
  bgColor = CRGB{0,0,0};
  currentPattern = LEDPattern::staticPattern;
  patternTimer = millis();
  patternState = INIT_PATTERN;

  // Old pattern that can be restored
  oldPatternState = INIT_PATTERN;
  oldMainColor = CRGB{255,0,0};
  oldBGColor = CRGB{0,0,0};
  oldPattern = LEDPattern::noPattern;

  patternDuration = -1;
  patternStartMillis = millis();
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
    case 'b': {
      avail = NEW_SENSOR_VALUE::NEW_BTN;
      BUTTON_PRESS presses = BUTTON_PRESS::PRESS;
      // Optional single press, double press, long press etc.
      if (_getter->available()) {
        char e = _getter->read();
        if (e > '0' && e > '9') {
          presses = (BUTTON_PRESS)(e - '0');
        }
      }
      if (buttonCB) buttonCB(presses);
      break;
    }
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

void SensorBoard::setAutoSensorMode(bool on) {
  if (on) _getter->println("!a");
  _getter->println("!o");
  this->autoMode = on;
}

bool SensorBoard::updateSensors(bool wait) {
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

void SensorBoard::update() {
  // Handle incoming data
  if (_getter->available()) handle();
  // Update leds
  updateLEDPattern();
}

// _____________________________________LED Stuff__________________________________________________
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
  CRGB c = CRGB{red, green, 0};
  for (int i = 0; i < NUM_LEDS; i++) {
    LED[i] = c;
  }
  updateLEDs();
}

void SensorBoard::allLEDs(CRGB c) {
  for (int i = 0; i < NUM_LEDS; i++) LED[i] = c;
}

void SensorBoard::setAllLEDs(CRGB c) {
  allLEDs(c);
  updateLEDs();
}

void SensorBoard::newLEDPattern(LEDPattern pattern, long duration, CRGB theFGColor, CRGB theBGColor) {
  // Save old pattern
  saveOldPattern();
  // Set new pattern variables
  patternDuration = duration;
  currentPattern = pattern;
  mainColor = theFGColor;
  bgColor = theBGColor;
  // Patern should init
  patternState = INIT_PATTERN;
  patternStartMillis = millis();
  // Update pattern once
  patternTimer = millis();
  updateLEDPattern();
}

void SensorBoard::saveOldPattern() {
  oldBGColor = bgColor;
  oldMainColor = mainColor;
  oldPattern = currentPattern;
}

void SensorBoard::restoreOldPattern() {
  bgColor = oldBGColor;
  mainColor = oldMainColor;
  currentPattern = oldPattern;
  // This is not nice but may be not necessary
  patternState = INIT_PATTERN;
  patternTimer = millis();
}

void SensorBoard::updateLEDs() {
  _getter->print('!L');
  for (int l = 0; l < NUM_LEDS; l++) {
    for (int c = 0; c < 3; c++) _getter->write(LED[l].raw[c]);
  }
  _getter->println();
}


void SensorBoard::updateStaticPattern(SensorBoard* obj) {
  obj->allLEDs(obj->mainColor);
  // Indicate that init has been performed (and static color has been set)
  obj->patternState++;
}

void SensorBoard::updateGlowPattern(SensorBoard* obj) {
  // on -1 init the glow pattern with black LEDs
  if (obj->patternState == INIT_PATTERN) {
    obj->allLEDs(CRGB{0,0,0});
    obj->patternState = GLOW_UP;
  }
  // Glow to main color here
  else if (obj->patternState == GLOW_UP) obj->fadeTowardColor(obj->mainColor, 10);
  // Glow to bg color here
  else if (obj->patternState == GLOW_DOWN) obj->fadeTowardColor(obj->bgColor, 10);
  // Check if mainColor is reached then glow back two background color
  if (obj->LED[0] == obj->mainColor) obj->patternState = GLOW_DOWN;
  else if (obj->LED[0] == obj->bgColor) obj->patternState = GLOW_UP;
}

void SensorBoard::updateBlinkPattern(SensorBoard* obj) {
  // On init start with black color
  CRGB color = CRGB{0,0,0};
  if (obj->patternState == INIT_PATTERN) obj->patternState = BLINK_ONE;
  // Foreground color is first state
  else if (obj->patternState == BLINK_ONE) {
    color = obj->mainColor;
    obj->patternState = BLINK_TWO;
  }
  // Background color is second state
  else if (obj->patternState == BLINK_TWO) {
    color = obj->bgColor;
    obj->patternState = BLINK_ONE;
  }
  obj->allLEDs(color);
}

void SensorBoard::updateActivePowerPattern(SensorBoard* obj) {
  if (obj->activePowerGetter) {
    float power = obj->activePowerGetter();
    obj->powerToLEDs(power);
  }
}

void SensorBoard::updateRoundPattern(SensorBoard* obj) {
  // On init set all colors to bg color
  if (obj->patternState == INIT_PATTERN) {
    obj->allLEDs(obj->bgColor);
  } else {
    obj->allLEDs(obj->bgColor);
    if (obj->patternState >= 0 && obj->patternState < NUM_LEDS) obj->LED[obj->patternState] = obj->mainColor;
  }
  obj->patternState++;
  if (obj->patternState == NUM_LEDS) obj->patternState = 0;
}

void SensorBoard::updateLEDPattern() {
  // Check if old pattern needs to be restored
  if (patternDuration != -1) {
    if (millis()-patternStartMillis > patternDuration) {
      restoreOldPattern();
      patternStartMillis = millis();
      patternDuration = -1;
    }
  }
  if (currentPattern >= LEDPattern::numberOfPatterns) return;
  // if pattern with no update time are specified only inititalize
  if (patternUpdateTimes[(int)currentPattern] == 0 && patternState != INIT_PATTERN) return;
  // Return if update time not reached or not inited yet
  if (patternState != INIT_PATTERN && millis() - patternTimer < patternUpdateTimes[(int)currentPattern]) return;
  // Handle the current pattern
  updatePattern[(int)currentPattern](this);
  // Update the timer and the leds
  patternTimer = millis();
  updateLEDs();
}

void SensorBoard::nblendU8TowardU8(uint8_t& cur, const uint8_t target, uint8_t amount) {
  if(cur == target) return;
  if(cur < target) {
    uint8_t delta = target - cur;
    delta = (((int)delta * (int)amount) >> 8) + ((delta&&amount)?1:0);
    // if (delta > amount) delta = amount;
    cur += delta;
  } else {
    uint8_t delta = cur - target;
    delta = (((int)delta * (int)amount) >> 8) + ((delta&&amount)?1:0);
    // if (delta > amount) delta = amount;
    cur -= delta;
  }
}

void SensorBoard::fadeTowardColor(const CRGB& bgColor, uint8_t fadeAmount) {
  for(int i = 0; i < NUM_LEDS; i++) {
    nblendU8TowardU8(LED[i].red,   bgColor.red,   fadeAmount);
    nblendU8TowardU8(LED[i].green, bgColor.green, fadeAmount);
    nblendU8TowardU8(LED[i].blue,  bgColor.blue,  fadeAmount);
  }
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
