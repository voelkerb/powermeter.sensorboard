/***************************************************
 Library for SensorBoard

 Feel free to use the code as it is.

 Benjamin Voelker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Computer Science
 ****************************************************/

#ifndef SENSORBOARD_h
#define SENSORBOARD_h


#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


#define NUM_LEDS 3
enum class LED_MODE {MANUAL = 0, POWER = 1};
enum class NEW_SENSOR_VALUE {NONE = 0, NEW_BTN = 1, NEW_TEMP = 2, NEW_HUM = 3, NEW_LIGHT = 4, NEW_PIR = 5, ACTIVE = 6, UNKNOWN = 6};
enum class BUTTON_PRESS {NONE=0, SINGLE=1, DOUBLE=2, LONG_START=3, RELEASE=4, PRESS=5};

class SensorBoard {
  #define SENSOR_WAIT_TIME 1000
  #define MAX_WATT 500
  
  public:
    SensorBoard(Stream * getter);
    bool init();
    void updateLEDs();
    enum NEW_SENSOR_VALUE handle(int timeout=-1);
    bool update(bool wait=false);
    bool updateLight(bool wait=false);
    bool updateTemp(bool wait=false);
    bool updateHum(bool wait=false);
    bool updatePIR(bool wait=false);

    void powerToLEDs(float power);
    void restartPattern();
    void resetPattern();
    void setBrightness(float brightness);

    uint8_t LED[NUM_LEDS][3];
    float humidity;
    float temperature;
    bool PIR;
    int light;
    bool active;
    LED_MODE ledMode;
    float brightness;

    void (*buttonCB)(BUTTON_PRESS);
    void (*tempCB)(float);
    void (*humCB)(float);
    void (*lightCB)(uint32_t);
    void (*PIRCB)(bool);

  private:
    template < typename TOut >
    TOut parse();
    Stream * _getter;
};

#endif
