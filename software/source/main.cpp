/* coding: utf-8
 *
 */
#include <xpcc/architecture/platform.hpp>
#include <avr/sleep.h>
#include <avr/power.h>

#define UART_ENABLED 0

// IO #########################################################################
//
//                     +------------+
// (MOTION/INT1) PD3  1|o           |24  PC1
//               PD4  2|            |23  PC0 (ADC0/VOLTAGE)
//               GND  3|            |22  ADC7
//               VCC  4|            |21  GND
//               GND  5|            |20  AREF
//               VCC  6|            |19  ADC6
//       (XLAT1) PB6  7|            |18  AVCC
//       (XLAT2) PB7  8|            |17  PB5 (SCK)
//                     +------------+
//
//                     +------------+
//              PD5   9|           o|32  PD2 (BUTTON/INT0)
//       (XLAT) PD6  10|            |31  PD1 (TXD)
//      (BLANK) PD7  11|            |30  PD0 (RXD)
//      (GSCLK) PB0  12|            |29  PC6 (RESET)
//              PB1  13|            |28  PC5
//      (VPROG) PB2  14|            |27  PC4
//       (MOSI) PB3  15|            |26  PC3
//       (MISO) PB4  16|            |25  PC2
//                     +------------+


GPIO__INPUT(RXD, D, 0);
GPIO__OUTPUT(TXD, D, 1);

GPIO__INPUT(BUTTON, D, 2);
GPIO__INPUT(MOTION, D, 3);

GPIO__OUTPUT(VPROG, B, 2);
GPIO__OUTPUT(XLAT, D, 6);
GPIO__OUTPUT(BLANK, D, 7);
GPIO__OUTPUT(GSCLK, B, 0);

GPIO__INPUT(VOLTAGE, C, 0);

GPIO__INPUT(RESET, C, 6);
GPIO__OUTPUT(SCK, B, 5);
GPIO__OUTPUT(MOSI, B, 3);
GPIO__INPUT(MISO, B, 4);


// LED FADER ##################################################################
#include <xpcc/driver/ui/led.hpp>

typedef xpcc::atmega::SpiMaster LedSpi;
typedef xpcc::TLC594X< 32, LedSpi, XLAT, VPROG > controller;

// 10 white LEDs in the front
uint8_t whiteChannels[10] = {15,14,13,12,11,31,30,29,28,27};
xpcc::led::TLC594XMultipleLed< controller > white(whiteChannels, 10);

// six red LEDs in the back, four are constant
uint8_t redChannels[4] = {4,5,9,7};
xpcc::led::TLC594XMultipleLed< controller > red(redChannels, 4);

// two are pulsing
uint8_t redPulsingChannels[2] = {10,8};
xpcc::led::TLC594XMultipleLed< controller > redPulsingLed(redPulsingChannels, 2);
xpcc::led::Pulse redPulsing(&redPulsingLed, 1458);

// two position lights
uint8_t positionChannels[2] = {0,3};
xpcc::led::TLC594XMultipleLed< controller > position(positionChannels, 2);

// two beacon lights
xpcc::led::TLC594XLed< controller > beaconLed(6);
xpcc::led::Indicator beacon(&beaconLed, 2467, 0.28f, 130, 200);

// two strobe lights
uint8_t strobeChannels[2] = {1,2};
xpcc::led::TLC594XMultipleLed< controller > strobeLed(strobeChannels, 2);
xpcc::led::DoubleIndicator strobe(&strobeLed, 1825, 0.06f, 0.075f, 0.05f, 0, 20);

// blue stripes
uint8_t blueChannels[7] = {20,21,22,23,24,25,26};
xpcc::led::TLC594XMultipleLed< controller > blue(blueChannels, 7);

// io light
xpcc::led::TLC594XLed< controller > io(16);

// UART #######################################################################
#if UART_ENABLED
typedef xpcc::atmega::BufferedUart0 Uart;
Uart uart(38400);

#	include <xpcc/io/iodevice_wrapper.hpp>
#	include <xpcc/io/iostream.hpp>
xpcc::IODeviceWrapper<Uart> device(uart);
xpcc::IOStream stream(device);
#	define UART_STREAM(x) stream << x << xpcc::endl
#	define UART(x) uart.write(x)
#else
#	define UART_STREAM(x)
#	define UART(x)
#endif


// TIMEOUTS ###################################################################
#include <xpcc/workflow.hpp>
const uint16_t fadeTime = 5000;
xpcc::Timeout<> fadeOutTimer(fadeTime);
xpcc::Timeout<> fadeInTimer(fadeTime);

bool halfBrightness = false;
const uint8_t lowBrightness = 2;
const uint8_t highBrightness = 16;

const uint16_t motionTimeout = 10000;
xpcc::Timeout<> motionTimer(motionTimeout);
bool inMotion = true;
bool inMotionPrev = false;

const uint16_t buttonLongPressTime = 1000;
const uint16_t buttonShortPressTime = 50;
xpcc::Timeout<> buttonLongTimer(buttonLongPressTime);
xpcc::Timeout<> buttonShortTimer(buttonShortPressTime);

enum
{
	BUTTON_NO_PRESS,
	BUTTON_SHORT_PRESS,
	BUTTON_LONG_PRESS,
} buttonStatus = BUTTON_SHORT_PRESS;
bool depressed;


// INTERRUPTS #################################################################
ISR(TIMER2_COMPA_vect)
{
	xpcc::Clock::increment();
	
	controller::writeChannels();
}

ISR(TIMER0_OVF_vect)
{
	BLANK::reset();
}

ISR(TIMER0_COMPB_vect)
{
	BLANK::set();
}

ISR(INT1_vect)
{
	// restart the motion timer with 10 seconds
	motionTimer.restart(10000);
	inMotion = true;
	
//	UART('m');
}

ISR(INT0_vect)
{
	// true button is "closed", false button is "open"
	depressed = !BUTTON::read();
	static bool depressedPrev = false;
	UART_STREAM("Button=" << depressed);
	
	if (depressed != depressedPrev)
	{
		depressedPrev = depressed;
		motionTimer.restart(10000);
		inMotion = true;
	
		if (depressed)
		{
			buttonShortTimer.restart(buttonShortPressTime);
			buttonLongTimer.restart(buttonLongPressTime);
			io.on();
		}
		else
		{
			io.off();
			if (buttonLongTimer.isExpired())
			{
				buttonStatus = BUTTON_LONG_PRESS;
				UART_STREAM("Long Press");
			}
			else
			{
				EIMSK |= (1 << INT1);
				if (buttonShortTimer.isExpired())
				{
					buttonStatus = BUTTON_SHORT_PRESS;
					UART_STREAM("Short Press");
				}
				else
				{
					buttonStatus = BUTTON_NO_PRESS;
					UART_STREAM("No Press");
				}
			}
		}
	}
}


MAIN_FUNCTION // ##############################################################
{
	// Pull-up on Mode, Button and Motion pins
	BUTTON::setInput(xpcc::atmega::PULLUP);
	MOTION::setInput(xpcc::atmega::PULLUP);
	RESET::setInput(xpcc::atmega::PULLUP);
	TXD::setOutput();
	RXD::setInput();
	VOLTAGE::setInput();
	BLANK::setOutput(xpcc::gpio::LOW);
	SCK::setOutput();
	MOSI::setOutput();
	MISO::setInput();
	VPROG::setOutput(xpcc::gpio::LOW);
	XLAT::setOutput(xpcc::gpio::LOW);
	
	LedSpi::initialize(LedSpi::MODE_0, LedSpi::PRESCALER_2);
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	// Set up interupts for the motion sensor and button
	// set external interrupt on any logical change for Button/Int0 and Motion/Int1
	EICRA = (1 << ISC00) | (1 << ISC10);
	// enable both external interrupts
	EIMSK = (1 << INT0) | (1 << INT1);
	
	// Fast PWM Mode, clear at Bottom
	TCCR0A = (1<<WGM00) | (1<<WGM01);
	// Grayscale counter going at 8000kHz/4096 = 1.953125kHz
	// this period should be slightly slower
	// 8000kHz / 64 / 65 = 1.92kHz
	TCCR0B = (1<<CS01) | (1<<CS00) | (1<<WGM02);
	OCR0A = 65;
	OCR0B = 64;
	// Enable Output Compare Match interrupt B
	TIMSK0 = (1<<OCIE0B) | (1<<TOIE0);
	
	// Enable 1ms interrupt
	// CTC Mode
	TCCR2A = (1<<WGM21);
	// 8000kHz / 64 / 125 = 1kHz = 1ms
	TCCR2B = (1<<CS22);
	OCR2A = 125;
	// Enable Overflow Interrupt
	TIMSK2 = (1<<OCIE2A);
	
	
	xpcc::atmega::enableInterrupts();
	UART_STREAM("\n\nRESTART\n");
	
	// 1.24V * 31.5 / (0.5kOhm) = 78.12mA
	// 64 / 78.12mA * 20mA = 16.41
	controller::initialize(0, highBrightness, true, false);
	
	
	while (1)
	{
		white.run();
		red.run();
		redPulsing.run();
		position.run();
		beacon.run();
		strobe.run();
		blue.run();
		io.run();
		
		if (motionTimer.isExpired())
		{
			inMotion = false;
		}
		
		if (buttonStatus == BUTTON_LONG_PRESS)
		{
			buttonStatus = BUTTON_NO_PRESS;
			// disable motion sensor
			EIMSK &= ~(1 << INT1);
			inMotion = false;
		}
		
		if (inMotion != inMotionPrev)
		{
			inMotionPrev = inMotion;
			
			if (inMotion)
			{
				UART_STREAM("switching on");
				white.on(fadeTime);
				red.on(fadeTime);
				blue.on(fadeTime);
				position.on(fadeTime);
				
				fadeInTimer.restart(fadeTime);
			}
			else
			{
				UART_STREAM("switching off");
				white.off(fadeTime);
				red.off(fadeTime);
				blue.off(fadeTime);
				position.off(fadeTime);
				
				redPulsing.stop();
				beacon.stop();
				strobe.stop();
				
				fadeOutTimer.restart(fadeTime);
			}
		}
		
		if (buttonStatus == BUTTON_SHORT_PRESS)
		{
			buttonStatus = BUTTON_NO_PRESS;
			
			uint8_t brightness = halfBrightness ? lowBrightness : highBrightness;
			controller::setAllDotCorrection(brightness, false);
			if (!halfBrightness)
			{
				controller::setDotCorrection(6, 2*highBrightness);
				controller::setDotCorrection(11, 0x3f);
				controller::setDotCorrection(12, 0x3f);
				controller::setDotCorrection(13, 0x3f);
				controller::setDotCorrection(14, 0x3f);
				controller::setDotCorrection(15, 0x3f);
				controller::setDotCorrection(27, 0x3f);
				controller::setDotCorrection(28, 0x3f);
				controller::setDotCorrection(29, 0x3f);
				controller::setDotCorrection(30, 0x3f);
				controller::setDotCorrection(31, 0x3f);
				controller::setDotCorrection(4, 6);
				controller::setDotCorrection(5, 6);
				controller::setDotCorrection(7, 6);
				controller::setDotCorrection(9, 6);
			}
			controller::setDotCorrection(16, 1);
			{
				xpcc::atomic::Lock lock;
				controller::writeDotCorrection();
			}
			UART_STREAM("brightness=" << brightness);
			halfBrightness = !halfBrightness;
		}
		
		if (depressed && buttonLongTimer.isExpired())
		{
			io.off();
		}
		
		if (inMotion && !beacon.isRunning() && fadeInTimer.isExpired())
		{
			redPulsing.start();
			beacon.start();
			strobe.start();
		}
		
		if (!inMotion && fadeOutTimer.isExpired())
		{
			// turn off all LED driver outputs
			BLANK::set(xpcc::gpio::HIGH);
			// go to deep sleep
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			sleep_mode();
			
			UART_STREAM("wakeup");
			// put mode back to idle
			set_sleep_mode(SLEEP_MODE_IDLE);
		}
		else
		{
			sleep_mode();
		}
	}
}
