# CH32V003-Matchbox-Binary-Clock
CH32V003 PCF8563 WS2812B learning project.
Clock in matchbox size with touch control

Binary clock, it has 16 pieces WS2812B arranged in a 4x4 matrix display showing binary coded time.

Touching the top, the CH32V003 gets a wake up interrupt from a TTP223 touch sensor, wakes from sleep, reads the RTC chip PCF8563 and displays the time on WS2812 chain. Waits 2 sec, turns off the WS2812 supply and goes back to sleep mode to save the battery. In this mode, the consumption is about 20 uA, it means that the battery is good for 30000 working hours (many years theoretically) with a single charge.

The color of the WS2812 is freely adjustable in the Arduino program, the default is red for the 1, yellow for 2, green 4, blue (purple) 8 binary weight. To read the time, you add the weights vertically. 

Display: The matrix of 16 WS2812B LEDs in series is available on Aliexpress. Connect 3 wires, Gnd, Vcc, DI data. The 470uF capacitor smoothes current peaks. Position the display with the solder pads up.

CH32V003 module : CH32V003F4P6 chip soldered on a 20 pin adapter PCB, the module runs on 24Mhz internal clock. The Vcc GND pin has 100nF + 470uF capacitors soldered on. Use the WCH-LinkE interface for programming, you need 3 wires : Vcc, 3V3, and SWIO on pin PD1. Unlike the AVR Arduinos, this chip needs no bootloader.

TTP223 :  Make the A jumper (active low output). Remove the LED or its resistor. Use no pins, the space is limited.
Available here: https://www.aliexpress.com

PCF8563 clock module : built on a SOP8 PCB, the module is very small.

TP4056 : Remove/replace the 1k2 resistor with 6k8.  Order the smallest possible TP4056 module. Use no pins, the space is limited.
Available here: https://www.aliexpress.com


Construction : Print the 3D parts. Use 0.25mm enamel wire for wiring. The WS matrix is followed by the CH32 board and the clock board in the case. There is a cavity for the TTP223 and the 6x6mm tactile switches. See picture.  
    
The battery compartment is made for a 37x50x7mm battery in this design. You may use any other type of Li-Ion battery, 3D print a box for it using my wedge assembly or simply glue it to the clock case. Drill or burn a hole for the wires to the Arduino.

What I learned from this project : using direct register control, mixing MounRiver Studio code into Arduino (it works), powersave mode which uses only 10-20 uA current, pin interrupt, driving the WS2812B without libraries. Avoid Arduino pinMode() instructions, drives powersave current up to 100uA. The instruction like GPIOC->OUTDR &= ~(1 << 3); executes faster than the MounRiver style GPIO control, like  GPIO_WriteBit(GPIOC, GPIO_Pin_3, Bit_RESET) and waaaay faster than digitalWrite().  
