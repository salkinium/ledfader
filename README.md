Standalone driver for 32 LEDs
=============================

This board should drive 32 LEDs using 12bit resolution at a ~2kHz frequency
using two TLC5940s controlled by a ATmega328.
There is one button and a vibration sensor, to changes modes and save power (both optional).
The serial can be accessed (so Arduino support should be possible).

The software is written using the [XPCC library][xpcc], for its TLC5940 driver,
gamma correction and workflow classes.


Organisation
------------

This repository is organized as follows:

-	The *hardware* folder contains the EAGLE schematics and layouts of the
	electronic assembly, with parts list and datasheets.
- 	The *software* folder contains the software for the microcontroller.

[xpcc]: http://xpcc.kreatives-chaos.com
