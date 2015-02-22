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
 * float_waveform.h
 *
 *  Created on: 17 Feb 2013
 *      Author: ntuckett
 */

#ifndef FLOAT_WAVEFORM_H_
#define FLOAT_WAVEFORM_H_

#include "fixed_point_math.h"

typedef struct float_oscillator_t
{
	float	frequency;
	float	level;

	float	phase;				// runs from 0 to 1
	fixed_t	phase_fixed;
	float	last_level;
} float_oscillator_t;

typedef struct
{
	int			sample_count;
	float		frequency;
	fixed_t		phase_step;
	float		*samples;
} float_waveform_t;

extern void float_generate_sine(float_waveform_t *waveform, float sample_rate, float frequency);
extern void waveform_float_procedural_sine(float_oscillator_t *osc, float *sample_buffer, int sample_count);
extern void waveform_float_procedural_sine_mix(float_oscillator_t *osc, float *sample_buffer, int sample_count);
extern void waveform_float_wavetable_sine(float_waveform_t *waveform, float_oscillator_t *osc, float *sample_buffer, int sample_count);
extern void waveform_float_wavetable_sine_mix(float_waveform_t *waveform, float_oscillator_t *osc, float *sample_buffer, int sample_count);

#endif /* FLOAT_WAVEFORM_H_ */
