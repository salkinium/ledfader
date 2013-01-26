LED driver hardware
===================

The ATmega328P can fade 32 LEDs using two TLC5940 12bit LED drivers.
A vibration sensor and a button can be used to implement a power saving
functions and mode changes.
RX and TX can be accesses for debugging, but it might also allow use of Arduino.


Parts List and Description
--------------------------

### Battery charger

Perhaps using [Sparkfuns USB Lipo charger](
https://www.sparkfun.com/products/10401)?
Might require some diodes.


### Battery

A 3.7V LiPo can be used **without** voltage regulator.


### Microcontroller

A ATmega328P in a 32-pin TQFP package is used.
The internal oscillator at 8MHz is used to reduce components.
A precise clock is not required, so it is okay.


### Tactile Switch

A light touch switch from Panasonic is used: EVQPS with J-bend terminals:
EVQPSG02K without Boss


### Vibration sensor

MVS 0608.02 is used.


### Other parts

Some 0603 and 0805 resistors and capacitors are used.

A RM 2.00 mm In-System Programming socket is used.

