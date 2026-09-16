#include "Toaster.h"

#include <rtl-sdr.h>

#include <iostream>


// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
/* 
TODO: Stages:
Stage 1: DDC (Digital Down Converter) - filter out trash
//Stage 2: Signal Conditioning
	- DC Blocking Filter
	- Automatic Gain Control (with feedback loop)
//Stage 3: Synchronization
	- Simple Carrier Frequency Offset (CFO) Correction
Stage 4: Demodulation
	- FM Demodulation (Phase Derivative)
Stage 5: Post Processing
	- De-emphasis Filter (Simple RC Lowpass)
*/


/*
Audio:
	recv
	filter
	decimation
	demodulation
	de-emphasis
	audio output
*/
int main()
{
	Toaster toaster;
	toaster.createToasterSubsystem();
	toaster.startToasterSubsystem();
	//toaster.stopToasterSubsystem();

	return 0;
}

