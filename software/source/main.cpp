/* coding: utf-8
 *
 */
#include <xpcc/architecture/platform.hpp>

#define DEBUG 0

// IO #########################################################################
//
//                     +------------+
// (MOTION/INT1) PD3  1|o           |24  PC1
//               PD4  2|            |23  PC0 (ADC0/VOLTAGE)
//               GND  3|            |22  ADC7
//               VCC  4|            |21  GND
//               GND  5|            |20  AREF
//               VCC  6|            |19  ADC6
//               PB6  7|            |18  AVCC
//               PB7  8|            |17  PB5 (SCK)
//                     +------------+
//
//                     +------------+
//       (XLAT) PD5   9|           o|32  PD2 (BUTTON/INT0)
//      (BLANK) PD6  10|            |31  PD1 (TXD)
//              PD7  11|            |30  PD0 (RXD)
//      (GSCLK) PB0  12|            |29  PC6 (RESET)
//              PB1  13|            |28  PC5
//              PB2  14|            |27  PC4
//       (MOSI) PB3  15|            |26  PC3
//       (MISO) PB4  16|            |25  PC2
//                     +------------+


GPIO__INPUT(RXD, D, 0);
GPIO__OUTPUT(TXD, D, 1);

GPIO__INPUT(BUTTON, D, 2);
GPIO__INPUT(MOTION, D, 3);

GPIO__OUTPUT(XLAT, D, 5);
GPIO__OUTPUT(BLANK, D, 6);
GPIO__OUTPUT(GSCLK, B, 0);

GPIO__INPUT(VOLTAGE, C, 0);

GPIO__INPUT(RESET, C, 6);
GPIO__OUTPUT(SCK, C, 2);
GPIO__OUTPUT(MOSI, B, 3);
GPIO__INPUT(MISO, B, 4);


// LED FADER ##################################################################
#include <xpcc/driver/pwm/tlc594x.hpp>

typedef xpcc::atmega::SpiMaster LedSpi;
typedef xpcc::TLC594X< 32, LedSpi, XLAT > controller;

#include <xpcc/driver/ui/led.hpp>
typedef xpcc::led::TLC594XLed< controller, 2 > Red;
typedef xpcc::led::TLC594XLed< controller, 3 > Green;
typedef xpcc::led::TLC594XLed< controller, 4 > Blue;

xpcc::led::Pulse<Green> pulse;
xpcc::led::Indicator<Blue> indicator;
xpcc::led::DoubleIndicator<Red> strobe;

// TIMEOUT ####################################################################
#include <xpcc/workflow.hpp>
xpcc::Timeout<> motionTimer(20);


// DEBUG ######################################################################
#if DEBUG
typedef xpcc::BufferedUart0 Uart;
SoftwareUart uart(115200);
#endif


// INTERRUPTS #################################################################
ISR(TIMER0_COMPA_vect)
{
	// increment by 1ms
	xpcc::Clock::increment();
	xpcc::atomic::Unlock unlock;
	
	pulse.run();
	indicator.run();
	strobe.run();
	
	controller::writeChannels();
}

ISR(INT1_vect)
{
	// restart the motion timer with 10 seconds
	motionTimer.restart(10000);
}

ISR(INT0_vect)
{
	
}


MAIN_FUNCTION // ##############################################################
{
	// Set up interupts for the motion sensor and button
	// set external interrupt on any logical change for Button/Int0 and Motion/Int1
	EICRA = (1 << ISC00) | (1 << ISC10);
	// enable both external interrupts
	EIMSK = (1 << INT0) | (1 << INT1);
	
	// Enable 1ms interrupt
	// CTC Mode
    TCCR0A = (1<<WGM01);
	// 8000kHz / 64 / 125 = 1kHz = 1ms
    TCCR0B = (1<<CS01)|(1<<CS00);
	OCR0A = 125;
    // Enable Overflow Interrupt
    TIMSK0 = (1<<OCIE0A);
	
	
	// Pull-up on Mode, Button and Motion pins
	BUTTON::setInput(xpcc::atmega::PULLUP);
	MOTION::setInput(xpcc::atmega::PULLUP);
	RXD::setInput();
	VOLTAGE::setInput();
	BLANK::setOutput(xpcc::gpio::LOW);
	SCK::setOutput();
	MOSI::setOutput();
	
	
	LedSpi::initialize(LedSpi::MODE_0, LedSpi::PRESCALER_8);
	
	controller::initialize(0, 63, true, false);
	controller::setDotCorrection(3, 10);
	controller::setDotCorrection(4, 20);
	controller::writeDotCorrection();
	
	// init is done, full power, Skotty!
	xpcc::atmega::enableInterrupts();
	
	pulse.pulse(20);
	indicator.indicate(200);
	strobe.indicate(20);
	
	while (1)
	{
		
	}
}
