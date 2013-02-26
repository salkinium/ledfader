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
//       (XLAT) PD5   9|           o|32  PD2 (BUTTON/INT0)
//      (BLANK) PD6  10|            |31  PD1 (TXD)
//              PD7  11|            |30  PD0 (RXD)
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
GPIO__OUTPUT(XLAT, D, 5);
GPIO__OUTPUT(BLANK, D, 6);
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
uint8_t whiteChannels[10] = {0,1,2,3,4,5,6,7,8,9};
xpcc::led::TLC594XMultipleLed< controller > white(whiteChannels, 10);

// six red LEDs in the back, four are constant
uint8_t redChannels[4] = {10,11,12,13};
xpcc::led::TLC594XMultipleLed< controller > red(redChannels, 4);

// two are pulsing
uint8_t redPulsingChannels[2] = {14,15};
xpcc::led::TLC594XMultipleLed< controller > redPulsingLed(redPulsingChannels, 2);
xpcc::led::Pulse redPulsing(&redPulsingLed, 1111);

// two position lights
uint8_t positionChannels[2] = {16,17};
xpcc::led::TLC594XMultipleLed< controller > position(positionChannels, 2);

// two beacon lights
uint8_t beaconChannels[2] = {18,19};
xpcc::led::TLC594XMultipleLed< controller > beaconLed(beaconChannels, 2);
xpcc::led::Indicator beacon(&beaconLed, 1567, 0.2f, 60, 100);

// two strobe lights
uint8_t strobeChannels[2] = {20,21};
xpcc::led::TLC594XMultipleLed< controller > strobeLed(strobeChannels, 2);
xpcc::led::DoubleIndicator strobe(&strobeLed, 1327, 0.06f, 0.075f, 0.05f, 0, 20);

// five indicator lights left
uint8_t indicatorLeftChannels[5] = {22,23,24,25,26};
xpcc::led::TLC594XMultipleLed< controller > indicatorLeftLed(indicatorLeftChannels, 5);
xpcc::led::Indicator indicatorLeft(&indicatorLeftLed);

// five indicator lights left
uint8_t indicatorRightChannels[5] = {27,28,29,30,31};
xpcc::led::TLC594XMultipleLed< controller > indicatorRightLed(indicatorRightChannels, 5);
xpcc::led::Indicator indicatorRight(&indicatorRightLed);


// TIMEOUT ####################################################################
#include <xpcc/workflow.hpp>
xpcc::Timeout<> motionTimer(10000);
bool inMotion = false;
bool inMotionPrev = false;


// UART ######################################################################
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
	
	UART('m');
}

ISR(INT0_vect)
{
	bool depressed = !BUTTON::read();
	(void) depressed;
	// mode changes?
	UART_STREAM("Button=" << depressed);
}


MAIN_FUNCTION // ##############################################################
{
	// Pull-up on Mode, Button and Motion pins
	BUTTON::setInput(xpcc::atmega::PULLUP);
	MOTION::setInput(xpcc::atmega::PULLUP);
	TXD::setOutput();
	RXD::setInput();
	VOLTAGE::setInput();
	BLANK::setOutput(xpcc::gpio::LOW);
	SCK::setOutput();
	MOSI::setOutput();
	MISO::setInput();
	VPROG::setOutput(xpcc::gpio::LOW);
	XLAT::setOutput(xpcc::gpio::LOW);
	
	LedSpi::initialize(LedSpi::MODE_0, LedSpi::PRESCALER_8);
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
	// 78.12mA / 64 * 20 = 24.4125
	controller::initialize(0, 24, true, true);
	
	while (1)
	{
		white.run();
		red.run();
		redPulsing.run();
		position.run();
		beacon.run();
		strobe.run();
		indicatorLeft.run();
		indicatorRight.run();
		
		if (motionTimer.isExpired())
		{
			inMotion = false;
		}
		
		if (inMotion != inMotionPrev)
		{
			inMotionPrev = inMotion;
			
			if (inMotion)
			{
				UART_STREAM("switching on");
				white.on(500);
				red.on(500);
				redPulsing.start();
				position.on(500);
				beacon.start();
				strobe.start();
			}
			else
			{
				UART_STREAM("switching off");
				white.off(500);
				red.off(500);
				redPulsing.stop();
				position.off(500);
				beacon.stop();
				strobe.stop();
			}
		}
		
//		sleep_mode();
	}
}
