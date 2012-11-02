/*
 * waveform.h
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#ifndef WAVEFORM_H_
#define WAVEFORM_H_

#include <sys/types.h>

typedef enum
{
	WAVE_FIRST = 0,

	WAVETABLE_SINE = 0,
	WAVETABLE_SAW,
	WAVETABLE_SAW_BL,
	WAVETABLE_SINE_LINEAR,
	WAVETABLE_SAW_LINEAR,
	WAVETABLE_SAW_LINEAR_BL,

	PROCEDURAL_SINE,
	PROCEDURAL_SAW,

	WAVE_LAST = PROCEDURAL_SAW,
	WAVE_COUNT
} waveform_type_t;

extern void waveform_initialise();

#endif /* WAVEFORM_H_ */
