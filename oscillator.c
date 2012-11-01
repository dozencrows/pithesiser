/*
 * oscillator.c
 *
 *  Created on: 31 Oct 2012
 *      Author: ntuckett
 */
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "oscillator.h"

void osc_init(oscillator_t* osc)
{
	osc->waveform 			= WAVETABLE_SINE;
	osc->frequency			= 440;
	osc->phase_accumulator 	= 0;
	osc->level 				= SHRT_MAX;
}

void osc_output(oscillator_t* osc, int sample_count, void *sample_data)
{
	waveform_generator_t *generator = &generators[osc->waveform];
	generator->output_func(&generator->definition, osc, sample_count, sample_data);
}

void osc_mix_output(oscillator_t* osc, int sample_count, void *sample_data)
{
	waveform_generator_t *generator = &generators[osc->waveform];
	generator->mix_func(&generator->definition, osc, sample_count, sample_data);
}
