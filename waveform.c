/*
 * waveform.c
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#include "waveform.h"
#include "waveform_wavetable.h"

waveform_generator_t generators[WAVE_COUNT];

void waveform_initialise()
{
	init_wavetables();
	init_wavetable_generator(WAVETABLE_SINE, &generators[WAVETABLE_SINE]);
	init_wavetable_generator(WAVETABLE_SAW, &generators[WAVETABLE_SAW]);
	init_wavetable_generator(WAVETABLE_SINE_LINEAR, &generators[WAVETABLE_SINE_LINEAR]);
	init_wavetable_generator(WAVETABLE_SAW_LINEAR, &generators[WAVETABLE_SAW_LINEAR]);
}
