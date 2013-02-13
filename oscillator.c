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
#include "waveform_internal.h"

void osc_init(oscillator_t* osc)
{
	osc->waveform 			= WAVETABLE_SINE;
	osc->frequency			= 440;
	osc->phase_accumulator 	= 0;
	osc->level 				= 0;
}

void osc_output(oscillator_t* osc, sample_t *sample_data, int sample_count)
{
	waveform_generator_t *generator = &generators[osc->waveform];
	if (generator->output_func != NULL)
	{
		generator->output_func(&generator->definition, osc, sample_count, sample_data);
	}
}

void osc_mix_output(oscillator_t* osc, sample_t *sample_data, int sample_count)
{
	waveform_generator_t *generator = &generators[osc->waveform];
	if (generator->mix_func != NULL)
	{
		generator->mix_func(&generator->definition, osc, sample_count, sample_data);
	}
}

void osc_mid_output(oscillator_t* osc, sample_t *sample_data, int sample_count)
{
	waveform_generator_t *generator = &generators[osc->waveform];
	if (generator->mid_func != NULL)
	{
		generator->mid_func(&generator->definition, osc, sample_count, sample_data);
	}
}
