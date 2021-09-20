[PowerMeter]: (https://github.com/voelkerb/powermeter)
[bopla]:(https://www.conrad.de/de/p/bopla-eletec-se-432-de-cee-stecker-gehaeuse-120-x-65-x-50-abs-polycarbonat-lichtgrau-graphitgrau-1-st-522228.html)

# powermeter.sensorboard
Expansion board for the [PowerMeter] including multiple environmental sensors, LEDs, and a push button.

The [PowerMeter] includes an extension header that exposes the ESP32's UART interface.
This interface can be used to add more environmental sensors to a [PowerMeter].
The developed sensor board can be stacked on the [PowerMeter] _PCB_ and the embedded sensors may be exposed to the outside of the housing. 
See the following example:

<p align="center">
  <img src="/docu/figures/PMSensorBoard.jpg" width="200px">
  <img src="/docu/figures/PMSensorBoard2.jpg" width="200px">
</p>


---

## Sensors
The board incorporates typical environmental sensors:
A DHT22 for temperature and humidity readings, a TSL2561 light sensor and PIR sensor to detect people's movements. 

## Feedback
For visual feedback, three WS2812B RGB LEDs are included. These can automatically visualize the current energy consumption via color (_eco-feedback_) or indicate states or errors via patterns.   

## Interfacing
The incorporated button allows to switch the [PowerMeter] relay. If you require other functionality, these can be added via double-press or long-press gestures as well.

## Housing
The housing is a modified version of the [bopla] housing used for the [PowerMeter]. You can either cut in holes or 3D print a version with corresponding cutouts and holders for the sensor board. The 3D files also include the button and a light sensor and LED cover which should be printed with transparent filament. 