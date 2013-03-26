LED driver hardware
===================

The ATmega328P can fade 32 LEDs using two TLC5940 12bit LED drivers.

Programmer
----------

The ATmega is programmed using the standard ISP interface, however to save space,
a smaller footprint is used.
On standard PCBs with 1.6mm thickness, the board fits exacly between to RM 2.00 pins,
so I just use a 2x3 RM 2.00 pin header to attach my programmer to it from the side.
This takes up a *lot* less space and the connection is secure enough.


Parts List and Description
--------------------------

### Battery charger

Using [Sparkfuns USB Lipo charger](
https://www.sparkfun.com/products/10401).


### Battery

This circuit can be powered by a standard 3.7V LiPo/LiIon rechargeable battery **without** voltage regulator.

### Microcontroller

A ATmega328P in a 32-pin TQFP package is used.

### Resonator

Small footprint, reasonable frequency stability of ±0.5% at 8MHz: CSTCE 8,00.

Some background on the maximum possible frequency:
The 3.7V LiPo battery voltage can be anywhere between 3V to 4.2V.

The ATmega328P is rated at 10Mhz @ 2.7V and 20Mhz @ 4.5V.
For 3V the maximum frequency is already above 10MHz, so we're fine.  

The lowest voltage for 8MHz is:  
(8MHz - 4MHz) * (2.7 - 1.8)V / (10 - 4)MHz + 1.8V = 2.4V

But the TLC5940 requires 3V minimum voltage, so never mind.

### Tactile Switch

A light touch switch from Panasonic is used:  
EVQPS with J-bend terminals: EVQPSG02K without Boss


### Vibration sensor

MVS 0608.02 is used.  
It's not widely available so you can leave it out.


### Other parts

Some 0603 and 0805 resistors and capacitors are used.

A RM 2.00 mm In-System Programming socket is used.

