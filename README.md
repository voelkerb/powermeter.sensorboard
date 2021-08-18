[PowerMeter]: (https://github.com/voelkerb/powermeter)

# powermeter.sensorboard
Expansion board for the [PowerMeter] including multiple environmental sensors, LEDs, and a push button.

The [PowerMeter] includes an extension header that exposes the ESP32's UART interface.
This interface can be used to add more environmental sensors to a [PowerMeter].
The developed sensor board can be stacked on the [PowerMeter] _PCB_ and the embedded sensors may be exposed to the outside of the housing. 
See the following example:


<img src="/docu/figures/PMSensorBoard.jpg" width="200px" align="left">
<img src="/docu/figures/PMSensorBoard2.jpg" width="200px" align="right">

---

## Sensors
The board incorporates typical environmental sensors:
A DHT22 for temperature and humidity readings, a TSL2561 light sensor and PIR sensor to detect people's movements. 

## Feedback
For visual feedback, three WS2812B RGB LEDs are included. These can automatically visualize the current energy consumption via color (_eco-feedback_) or indicate states or errors via patterns.   

## Interfacing
The incorporated button allows to switch the [PowerMeter] relay. If you require other functionality, these can be added via double-press or long-press gestures as well.