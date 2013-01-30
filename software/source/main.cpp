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
// 10 white LEDs in the front
uint8_t whiteChannels[10] = {0,1,2,3,4,5,6,7,8,9};
typedef xpcc::led::TLC594XLed< controller, whiteChannels, 10 > White;
White white;

// six red LEDs in the back, four are constant
uint8_t redChannels[4] = {10,11,12,13};
typedef xpcc::led::TLC594XLed< controller, redChannels, 4 > Red;
Red red;
// two are pulsing
uint8_t redPulsingChannels[2] = {14,15};
typedef xpcc::led::TLC594XLed< controller, redPulsingChannels, 2 > RedPulsing;
xpcc::led::Pulse<RedPulsing> redPulsing(800);

// two position lights
uint8_t positionChannels[2] = {16,17};
typedef xpcc::led::TLC594XLed< controller, positionChannels, 2 > Position;
Position position;

// two beacon lights
uint8_t beaconChannels[2] = {18,19};
typedef xpcc::led::TLC594XLed< controller, beaconChannels, 2 > Beacon;
xpcc::led::Indicator<Beacon> beacon(1300, 0.2f, 75, 100);

// two strobe lights
uint8_t strobeChannels[2] = {20,21};
typedef xpcc::led::TLC594XLed< controller, strobeChannels, 2 > Strobe;
xpcc::led::DoubleIndicator<Strobe> strobe(1700, 0.1f, 0.2f, 0.1f, 20, 60);

// five indicator lights left
uint8_t indicatorLeftChannels[5] = {22,23,24,25,26};
typedef xpcc::led::TLC594XLed< controller, indicatorLeftChannels, 5 > IndicatorLeft;
xpcc::led::Indicator<IndicatorLeft> indicatorLeft;

// five indicator lights left
uint8_t indicatorRightChannels[5] = {27,28,29,30,31};
typedef xpcc::led::TLC594XLed< controller, indicatorRightChannels, 5 > IndicatorRight;
xpcc::led::Indicator<IndicatorRight> indicatorRight;


// TIMEOUT ####################################################################
#include <xpcc/workflow.hpp>
xpcc::Timeout<> motionTimer(10000);
bool inMotion = false;
bool inMotionPrev = false;


// DEBUG ######################################################################
#if DEBUG
typedef xpcc::BufferedUart0 Uart;
SoftwareUart uart(115200);
#endif


// INTERRUPTS #################################################################
ISR(TIMER0_COMPA_vect)
{
	xpcc::Clock::increment();
	
	controller::writeChannels();
}

ISR(INT1_vect)
{
	// restart the motion timer with 10 seconds
	motionTimer.restart(10000);
	inMotion = true;
}

ISR(INT0_vect)
{
	// mode changes?
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
	TXD::setOutput();
	RXD::setInput();
	VOLTAGE::setInput();
	BLANK::setOutput(xpcc::gpio::LOW);
	SCK::setOutput();
	MOSI::setOutput();
	MISO::setInput();
	
	
	LedSpi::initialize(LedSpi::MODE_0, LedSpi::PRESCALER_2);
	controller::initialize(0, 63, true, false);
	
	
	xpcc::atmega::enableInterrupts();
	
	
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
				white.on(500);
				red.on(500);
				redPulsing.start();
				position.on(500);
				strobe.start();
			}
			else
			{
				white.off(500);
				red.off(500);
				redPulsing.stop();
				position.off(500);
				strobe.stop();
			}
		}
	}
}
