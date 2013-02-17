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
//extern void waveform_float_procedural_sine(float_oscillator_t *osc, float *sample_buffer, int sample_count);
extern void waveform_float_procedural_sine_mix(float_oscillator_t *osc, float *sample_buffer, int sample_count);
extern void waveform_float_wavetable_sine(float_waveform_t *waveform, float_oscillator_t *osc, float *sample_buffer, int sample_count);
extern void waveform_float_wavetable_sine_mix(float_waveform_t *waveform, float_oscillator_t *osc, float *sample_buffer, int sample_count);

#endif /* FLOAT_WAVEFORM_H_ */
