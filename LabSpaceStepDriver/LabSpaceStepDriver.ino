
#include "PinsConfig.h"
#include "Command.h"
#include "StepperMotor.h"

volatile int iEncoder = 0;


String aState[4] = { "EHLO\n", "IDLE %d\n", "ROTT %d\n", "ALRM\n"};

ActiveState activeState;


void RotationDoneCallback();

StepMotor stepMotor(RotationDoneCallback);

ISR(TIMER1_COMPA_vect) // timer interrupt service routine
{
	stepMotor.timer_routine();
}


ISR(INT1_vect)		   // encoder
{
	if (!digitalRead(PIN_STEPDRIVER_ENC))
		iEncoder--;
	else
		iEncoder++;
}

// interrupt overflows every ~4ms => charOverflow overflows once per second
char charOverflow = 0;
ISR(TIMER2_OVF_vect)	// timer interrupt service routine
{
	if (charOverflow == 0)
		digitalWrite(LED_BUILTIN, HIGH);
	else if (charOverflow == 80)			// 80*4ms = 0.32s
		digitalWrite(LED_BUILTIN, LOW);

	charOverflow++;
}


// the setup function runs once when you press reset or power the board
void setup() {
	// initialize digital pin LED_BUILTIN as an output.
	pinMode(LED_BUILTIN, OUTPUT);

	activeState = STATE_EHLO;

	// Serial to USB
	Serial.begin(9600);
	delay(500);

	// Initial greeting marker
	Serial.print(aState[activeState]);


#pragma region Configure stepdriver pins
	digitalWrite(PIN_STEPDRIVER_EN,	   LOW);		// pull-up disabled
	pinMode(PIN_STEPDRIVER_EN,         OUTPUT);     // switch Low

	digitalWrite(PIN_STEPDRIVER_STEP,  HIGH);		// enable-pull up
	pinMode(PIN_STEPDRIVER_STEP,       OUTPUT);		// switch High

	digitalWrite(PIN_STEPDRIVER_DIR,   LOW);		// disable pull-up
	pinMode(PIN_STEPDRIVER_DIR,        OUTPUT);     // switch Low

	pinMode(PIN_STEPDRIVER_ALARM,      INPUT_PULLUP);

	pinMode(PIN_STEPDRIVER_ENC_INT1,   INPUT);
	pinMode(PIN_STEPDRIVER_ENC,	       INPUT);
#pragma endregion

	cli(); // disable all interrupts

#pragma region set up Timer1 for StepDriver control
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;

	TCCR1B |= bit(WGM12);              // Clear Timer on Compare Match mode ("CTC" mode)
	TIMSK1 |= bit(OCIE1A);             // enable timer Output Compare Register A Match interrupt
#pragma endregion


#pragma region set up Timer2 for LED control
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2  = 0;

	// WGM20,21,22 = 0					 // Clear Timer on Overflow ("Normal" mode)
	TIMSK2 |= bit(TOIE2);                // enable timer Overflow interrupt

	// Start Timer2: 256 prescaler clock
	TCCR2B = ((TCCR2B) & 0b11111000) | bit(CS21) | bit(CS22);
#pragma endregion


#pragma region set up External interupt 1
	EICRA |= bit(ISC11);	             // Rising edge
	EIMSK |= bit(INT1);				     // External int1
#pragma endregion

	sei(); // enable all interrupts

	activeState = STATE_IDLE;

}

// the loop function runs over and over again forever
void loop() {

	ProcessCommand();

	delay(10);
}