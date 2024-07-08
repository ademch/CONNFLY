
#include "StepperMotor.h"
#include "Command.h"

// Theoretical background:
// 220 deg per 90min -> 2.44 deg/min -> 1.358 1.8deg/min -> 0.022 1.8 deg/s or Pulses per second
// Microstepping 20 increases pulses rate to 0.452 Pulses per second
// Microstepping 5  increases pulses rate to 0.113 Pulses per second

// From the other side
// 16000000 pulses per second / 1024 prescaler / 256*256 (16 bit timer) = 0.23 Pulses per second

// V=at, V=a*1
// S is in pulses:
// S=at^2/2 => S=vt^2/2 => S= V/2 * t^2, time of acceleartion is under 1 second =>
// Number of steps when t aims to 1s equals S = V/2
// t = sqrt(2*S/V), S = 1...V/2


// The function calculating marching parameters including acc and decc in 1s
// iVelocity is provided in [pulses per second] (not in 1.8 deg per second)
//
// PPS give ability to have finer control than 1.8 DPS (using e.g. microstepping or gearbox)
//
void StepMotor::MarchNSteps(unsigned int _i1_8_StepCount, float _fVelocity, bool _bCW)
{
	if (mState != IDLE) return;

	// save state
	i1_8_StepCount = _i1_8_StepCount;
	bCW            = _bCW;
	fVelocity      = _fVelocity;

	unsigned int iStepCount = _i1_8_StepCount * STEP_MICROSTEP;

	// automatic trunc here produces desirable effect of determining whether the speed provides time for acceleration/deacceleration pulses
	iACC_STEP_NUMBER = _fVelocity / 2.0f;

	if (iStepCount < iACC_STEP_NUMBER * 2) {
		iStepsAcc = iStepCount / 2;
		iStepsDec = iStepCount - iStepsAcc;
		iStepsMarch = 0;
	}
	else {
		iStepsAcc   = iACC_STEP_NUMBER;
		iStepsDec   = iACC_STEP_NUMBER;
		iStepsMarch = iStepCount - iACC_STEP_NUMBER * 2;
	}

	if (bCW) {
		digitalWrite(pin_dir, HIGH);		// clockwise
		iRotationDir = 1;
	}
	else {
		digitalWrite(pin_dir, LOW);			// counterclockwise
		iRotationDir = -1;
	}
	delayMicroseconds(20);					// signal sent at 50 kHz having 200kHz described as max allowed 

	mState = ACC;
	iSeq   = 0;

	OCR1A = 100;						    // 100 cycles when the timer is to be called

	// Start Timer1: 1024 prescaler clock
	TCCR1B = (TCCR1B & 0b11111000) | bit(CS12) | bit(CS10);

	// CS12 CS11 CS10
	//  0    0    0   no clock source
	//  0    0    1   x1 prescaler
	//  0    1    0   x8 prescaler
	//  0    1    1   x64 prescaler
	//  1    0    0   x256 prescaler
	//  1    0    1   x1024 prescaler
	//  1    1    0   Ext clock on pin T1. Clock falling edge
	//  1    1    1   Ext clock on pin T1. Clock rising edge
}



void StepMotor::SendStepPulseToDrive()
{
	digitalWrite(pin_step, HIGH);
		delayMicroseconds(20);		   // signal sent at 50 kHz having 200kHz described as max allowed 
	digitalWrite(pin_step, LOW);
}


void StepMotor::SetTimerVal(unsigned int val)
{
	OCR1A = val;

	// update current position
	iCurPos += iRotationDir;

	//Serial.print("iSeq=");
	//Serial.print(iSeq);
	//Serial.print(" timer_val=");
	//Serial.println(val);
}



// timer interrupt service routine
void StepMotor::timer_routine()
{
	if (mState == IDLE) return;

	if (mState == ACC)
	{
		if (iSeq >= iStepsAcc)
		{
			//Serial.println("acc->march");

			iSeq = 0;
			mState = MARCH;
		}
		else
		{
			//Serial.println("acc");

			SendStepPulseToDrive();
			//SetTimerVal(pgm_read_word_near(aSpeedProfile + iSeq));

			// Calculate time needed to go from the current to the next pulse
			// fT = sqrt(2*S2/V) - sqrt(2*S1/V) => fT = (sqrt(S2) - sqrt(S1)) / sqrt(V/2)
			float fT = (sqrtf(float(iSeq + 1)) - sqrtf(float(iSeq))) / sqrt(iACC_STEP_NUMBER);

			SetTimerVal(TIMER_TICKS_PERSECOND * fT);

			iSeq++;
		}
	}
	if (mState == MARCH)
	{
		if (iSeq >= iStepsMarch)
		{
			//Serial.println("march->dec");

			iSeq   = 0;
			mState = DECC;
		}
		else
		{
			//Serial.println("march");

			SendStepPulseToDrive();
			//SetTimerVal(pgm_read_word_near(aSpeedProfile + STEP_NUMBER - 1));	// use the last value as the fastest speed

			uint16_t timerVal = TIMER_TICKS_PERSECOND / fVelocity;
			SetTimerVal(timerVal);

			iSeq++;
		}
	}
	if (mState == DECC)
	{
		if (iSeq >= iStepsDec)
		{
			TCCR1B &= ~(bit(CS12) | bit(CS11) | bit(CS10));	// stop timer (clock source=none)

			//Serial.println("Done");

			iSeq   = 0;
			mState = IDLE;

			if (RotationDoneCallback) RotationDoneCallback();
		}
		else
		{
			//Serial.println("dec");

			SendStepPulseToDrive();
			//SetTimerVal(pgm_read_word_near(aSpeedProfile + iStepsDec - 1 - iSeq));

			// Calculate time needed to go between the current and the next pulse backwards
			// fT   =   sqrt(2*S2/V) - sqrt(2*S1/V) => fT = (sqrt(S2) - sqrt(S1)) / sqrt(V/2)
			float fT = (sqrtf(float(iStepsDec - iSeq)) - sqrtf(float(iStepsDec - 1 - iSeq))) / sqrt(iACC_STEP_NUMBER);

			SetTimerVal(TIMER_TICKS_PERSECOND * fT);

			iSeq++;
		}
	}
}

// Stop the rotation giving time to fully accelerate and fully deccelarate
void StepMotor::MarchStopPremature()
{
	if (mState != IDLE) iStepsMarch = 0;
}


// Stop now without decceleration
void StepMotor::MarchStopHard()
{
	if (mState != IDLE)
	{
		TCCR1B &= ~(bit(CS12) | bit(CS11) | bit(CS10));	// stop timer (clock source=none)

		iSeq   = 0;
		mState = IDLE;
	}
}

// When the motor is stopping after stop command and the user has commanded
// to resume rotation with the same params
void StepMotor::CatchUpDecceleratingMotor()
{
	if (mState == DECC)
	{
		mState = ACC;
		iSeq   = iStepsAcc - iSeq;
	}
}
