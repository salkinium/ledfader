LED drivers software
====================

The software is an example which manages all the LEDs on my bike helmet.
There is also some more advances stuff in there like sleep modes, which I use to minimize power use when the helmet is not in use.

You can/should adapt the firmware to your needs.
If you do not want to use XPCC, you should also be able to flash an Arduino bootloader on there (fingers crossed).


Firmware
--------

The sources are compiled using the [XPCC library][xpcc], which is used for
GPIO, TLC5940 Drivers, Processing, Build System and Serial Debugging.  
Enter the `source` directory

	$ cd /path/to/bookla/software/source

To compile, execute:

	$ scons

The fuses are set at 0xad (low), 0xd9 (high), 0x07 (extended).  
To set the fuses on the microcontroller, execute:

	$ scons fuse

You can also set the fuses manually using this command:

	$ avrdude -p atmega328p -c avrispmkii -P usb -u \
	-U lfuse:w:0xad:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m

To flash the binary onto the microcontroller, execute:

	$ scons program


Installing XPCC
---------------

XPCC is provided as a git submodule, to use it run this in the root `ledfader/`
directory:

	$ git submodule init
	$ git submodule update


To install the XPCC build system on OS X (tested on Lion/Mountain Lion):

1.	Install [Homebrew][]:  
	`$ ruby -e "$(curl -fsSL https://raw.github.com/mxcl/homebrew/go)"`
2.	Install some dependencies (requires Python 2.x):  
	`$ brew install scons python`  
	`$ pip install lxml jinja2`
3.	Install the latest precompiled version of [avr-gcc from ObDev][obdev].


To install the XPCC build system on Linux (tested on Ubuntu 12.04):

	$ sudo apt-get update
	$ sudo apt-get install python scons python-jinja2 python-lxml \
	gcc-avr binutils-avr avr-libc avrdude


Follow the information on installing [the XPCC build system here][xpcc-install].

[xpcc]: http://xpcc.kreatives-chaos.com/
[homebrew]: http://mxcl.github.com/homebrew/
[obdev]: http://www.obdev.at/products/crosspack/download.html
[xpcc-install]: http://xpcc.kreatives-chaos.com/install.html
