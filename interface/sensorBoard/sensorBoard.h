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

enum class LEDPattern {
  staticPattern = 0,
  blinkPattern = 1,
  roundPattern = 2,
  glowPattern = 3,
  activePowerPattern = 4,
  numberOfPatterns = 5
};

// CRGB Struct
struct CRGB {
  union {
		struct {
      union {
        uint8_t r;
        uint8_t red;
      };
      union {
        uint8_t g;
        uint8_t green;
      };
      union {
        uint8_t b;
        uint8_t blue;
      };
    };
    uint8_t raw[3];
  };
};

// Some easy defined colors
#define COLOR_BLACK CRGB{0,0,0}
#define COLOR_GREY CRGB{64,64,64}
#define COLOR_RED CRGB{255,0,0}
#define COLOR_GREEN CRGB{0,255,0}
#define COLOR_BLUE CRGB{0,0,255}
#define COLOR_ORANGE CRGB{255,255,0}
#define COLOR_PINK CRGB{255,0,255}

/// Test two CRGB for equality
inline __attribute__((always_inline)) bool operator== (const CRGB& lhs, const CRGB& rhs) { return (lhs.r == rhs.r) && (lhs.g == rhs.g) && (lhs.b == rhs.b); }
inline __attribute__((always_inline)) bool operator!= (const CRGB& lhs, const CRGB& rhs) { return !(lhs == rhs); }

class SensorBoard {
  #define SENSOR_WAIT_TIME 1000
  #define MAX_WATT 500
  #define IDLE_WATT 2
  
  public:
    SensorBoard(Stream * getter, void (*logFunc)(const char * msg, ...)=NULL);
    bool init();

    // Update LEDs and Serial communication
    void update();
    // Update sensors
    bool updateSensors(bool wait=false);
    bool updateLight(bool wait=false);
    bool updateTemp(bool wait=false);
    bool updateHum(bool wait=false);
    bool updatePIR(bool wait=false);
    void setAutoSensorMode(bool on);
    // Handle serial connection
    enum NEW_SENSOR_VALUE handle(int timeout=-1);

    void setBrightness(float brightness);
    // Set new LED pattern
    void newLEDPattern(LEDPattern pattern, long duration, CRGB theFGColor, CRGB theBGColor);
    // Easy wrappers
    void setRainbow(long duration=-1);
    void setDots(int dots, CRGB color, CRGB bgColor=COLOR_BLACK, long duration=-1);
    void setDots(int dots, CRGB color, long duration);
    void displayPowerColor(long duration=-1);
    void setIndividualColors(CRGB *colors, size_t n, bool fade=false, long duration=-1);
    void setColor(CRGB color, bool fade);
    void setColor(CRGB color, int duration);
    void setColor(CRGB color, long duration);
    void setColor(CRGB color, long duration=-1, bool fade=false);
    void glow(CRGB color, CRGB bgColor=COLOR_BLACK, long duration=-1);
    void glow(CRGB color, long duration);
    void blink(CRGB color, CRGB bgColor=COLOR_BLACK, long duration=-1);
    void blink(CRGB color, long duration);

    float humidity;
    float temperature;
    bool PIR;
    int light;
    bool active;
    bool autoMode;
    float brightness;

    void (*buttonCB)(BUTTON_PRESS);
    void (*tempCB)(float);
    void (*humCB)(float);
    void (*lightCB)(uint32_t);
    void (*PIRCB)(bool);
    float (*activePowerGetter)(void);

  private:

    // LED array
    CRGB LED[NUM_LEDS];
    // Setting all LEDs to same color
    void allLEDs(CRGB c);
    void setAllLEDs(CRGB c);
    // Update the led array
    void updateLEDs();

    // pattern states
    #define INIT_PATTERN -1
    #define GLOW_UP 0
    #define GLOW_DOWN 1
    #define BLINK_ONE 0
    #define BLINK_TWO 1
    #define ROUND_ONE 0
    #define ROUND_TWO 1

    // Speeds for pattern updates in ms
    #define GLOW_STEP 50
    #define BLINK_STEP 500
    #define ROUND_STEP 200
    #define UP_DOWN_STEP 300
    #define ACTIVE_POWER_UPDATE 5000

    // Array holding helper functions
    typedef void (* PatternFunction)(SensorBoard *obj);
    PatternFunction updatePattern[(int)LEDPattern::numberOfPatterns];
    // Array holding update times
    unsigned int patternUpdateTimes[(int)LEDPattern::numberOfPatterns];

    // Static LED update helper functions one for each pattern
    static void updateStaticPattern(SensorBoard *obj);
    static void updateBlinkPattern(SensorBoard *obj);
    static void updateRoundPattern(SensorBoard *obj);
    static void updateGlowPattern(SensorBoard *obj);
    static void updateActivePowerPattern(SensorBoard *obj);

    // LED helper functions
    void nblendU8TowardU8(uint8_t& cur, const uint8_t target, uint8_t amount);
    void fadeTowardColor(const CRGB& bgColor, uint8_t fadeAmount);
    // Updates the curretn led pattern
    void updateLEDPattern();
    // Converts given power to LED color
    void powerToLEDs(float power);

    // Saves old pattern
    void saveOldPattern();
    // Restores old pattern
    void restoreOldPattern();

    // Duration of current pattern
    long patternDuration;
    // Time current pattern started
    long patternStartMillis;

    int patternState;
    CRGB mainColor;
    CRGB bgColor;
    LEDPattern currentPattern;

    CRGB oldMainColor;
    CRGB oldBGColor;
    LEDPattern oldPattern;

    bool fadeUpdate;
    bool preSet;
    long patternTimer;

    template < typename TOut >
    TOut parse();
    Stream * _getter;


    void (*_logFunc)(const char * msg, ...);
};

#endif
