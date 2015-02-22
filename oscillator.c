// Pithesiser - a software synthesiser for Raspberry Pi
// Copyright (C) 2015 Nicholas Tuckett
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
	osc->last_level			= 0;
}

void osc_output(oscillator_t* osc, sample_t *sample_data, int sample_count)
{
	waveform_generator_t *generator = &generators[osc->waveform];
	if (generator->output_func != NULL)
	{
		generator->output_func(&generator->definition, osc, sample_data, sample_count);
	}
}

void osc_mix_output(oscillator_t* osc, sample_t *sample_data, int sample_count)
{
	waveform_generator_t *generator = &generators[osc->waveform];
	if (generator->mix_func != NULL)
	{
		generator->mix_func(&generator->definition, osc, sample_data, sample_count);
	}
}

void osc_mid_output(oscillator_t* osc, sample_t *sample_data, int sample_count)
{
	waveform_generator_t *generator = &generators[osc->waveform];
	if (generator->mid_func != NULL)
	{
		generator->mid_func(&generator->definition, osc, sample_data, sample_count);
	}
}
