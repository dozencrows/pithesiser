/*
 * waveform_wavetable.c
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "waveform_wavetable.h"
#include "oscillator.h"

#define WAVETABLE_SAMPLE_RATE		44100.0f
#define WAVETABLE_BASE_FREQUENCY	110.0f

typedef struct
{
	int		sample_count;
	int		frequency;
	int32_t	phase_limit;
	int16_t	*samples;
	int16_t	*linear_deltas;
} waveform_t;

static int wavetable_initialised = 0;
static waveform_t sine_wave;
static waveform_t saw_wave;

#define WT_CALC_PHASE_STEP(phase_step, osc, waveform) 	int32_t phase_step = ((int32_t)osc->frequency << FIXED_PRECISION) / (int32_t)waveform->frequency;

#define WT_GET_SAMPLE(osc, sample) 		   			int wave_index = osc->phase_accumulator >> FIXED_PRECISION; \
													int32_t sample = waveform->samples[wave_index]

#define WT_LINEAR_INTERP(waveform, osc, sample) 	sample += (((int32_t) waveform->linear_deltas[wave_index]) * (osc->phase_accumulator & FIXED_FRACTION_MASK)) >> FIXED_PRECISION

#define WT_ADVANCE_PHASE(osc, waveform, phase_step)	osc->phase_accumulator += phase_step; 					\
													if (osc->phase_accumulator > waveform->phase_limit) 	\
														osc->phase_accumulator -= waveform->phase_limit

static void wavetable_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	waveform_t *waveform = (waveform_t*) generator->waveform_data;

	if (waveform != NULL)
	{
		WT_CALC_PHASE_STEP(phase_step, osc, waveform);
		int16_t *sample_ptr = (int16_t*) sample_data;

		while (sample_count > 0)
		{
			WT_GET_SAMPLE(osc, sample);
			if (generator->flags & GENFLAG_LINEAR_INTERP)
			{
				WT_LINEAR_INTERP(waveform, osc, sample);
			}
			SCALE_AMPLITUDE(osc, sample);
			STORE_SAMPLE(sample, sample_ptr);
			WT_ADVANCE_PHASE(osc, waveform, phase_step);
			sample_count--;
		}
	}
}

static void wavetable_mix_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	waveform_t *waveform = (waveform_t*) generator->waveform_data;

	if (waveform != NULL)
	{
		WT_CALC_PHASE_STEP(phase_step, osc, waveform);
		int16_t *sample_ptr = (int16_t*) sample_data;

		while (sample_count > 0)
		{
			WT_GET_SAMPLE(osc, sample);
			if (generator->flags & GENFLAG_LINEAR_INTERP)
			{
				WT_LINEAR_INTERP(waveform, osc, sample);
			}
			SCALE_AMPLITUDE(osc, sample);
			MIX((int32_t)*sample_ptr, sample, mixed);
			STORE_SAMPLE(mixed, sample_ptr);
			WT_ADVANCE_PHASE(osc, waveform, phase_step);
			sample_count--;
		}
	}
}

static void generate_deltas(waveform_t *waveform)
{
	int i;

	waveform->linear_deltas = malloc(waveform->sample_count * sizeof(waveform->linear_deltas[0]));

	for (i = 0; i < waveform->sample_count - 1; i++)
	{
		waveform->linear_deltas[i] = waveform->samples[i + 1] - waveform->samples[i];
	}

	waveform->linear_deltas[i] = waveform->samples[0] - waveform->samples[i];
}

static void generate_sine(waveform_t *waveform, float sample_rate, float frequency)
{
	static float max_phase = M_PI * 2.0f;
	float phase_step = max_phase * frequency / sample_rate;

	waveform->frequency = frequency;
	waveform->sample_count = roundf(max_phase / phase_step);
	waveform->phase_limit = waveform->sample_count << FIXED_PRECISION;
	waveform->samples = malloc(waveform->sample_count * sizeof(waveform->samples[0]));

	float phase;

	for (int i = 0; i < waveform->sample_count; i++)
	{
		phase = i * phase_step;
		waveform->samples[i] = sinf(phase) * SHRT_MAX;
	}

	generate_deltas(waveform);
}

static void generate_saw(waveform_t *waveform, float sample_rate, float frequency)
{
	float phase_step = frequency / sample_rate;

	waveform->frequency = frequency;
	waveform->sample_count = roundf(1.0f / phase_step);
	waveform->phase_limit = waveform->sample_count << FIXED_PRECISION;
	waveform->samples = malloc(waveform->sample_count * sizeof(waveform->samples[0]));

	for (int i = 0; i < waveform->sample_count; i++)
	{
		float phase = i * phase_step;
		waveform->samples[i] = (1.0f - phase * 2.0f) * SHRT_MAX;
	}

	generate_deltas(waveform);
}

void init_wavetables()
{
	if (!wavetable_initialised)
	{
		generate_sine(&sine_wave, WAVETABLE_SAMPLE_RATE, WAVETABLE_BASE_FREQUENCY);
		generate_saw(&saw_wave, WAVETABLE_SAMPLE_RATE, WAVETABLE_BASE_FREQUENCY);
		wavetable_initialised = 1;
	}
}

void init_wavetable_generator(waveform_type_t waveform_type, waveform_generator_t *generator)
{
	u_int32_t flags = GENFLAG_NONE;
	waveform_t *waveform = NULL;

	switch (waveform_type)
	{
		case WAVETABLE_SINE_LINEAR:
			flags = GENFLAG_LINEAR_INTERP;
			waveform = &sine_wave;
			break;

		case WAVETABLE_SINE:
			waveform = &sine_wave;
			break;

		case WAVETABLE_SAW_LINEAR:
			flags = GENFLAG_LINEAR_INTERP;
			waveform = &saw_wave;
			break;

		case WAVETABLE_SAW:
			waveform = &saw_wave;
			break;

		default:
			break;
	}

	if (waveform != NULL)
	{
		generator->definition.flags = flags;
		generator->definition.waveform_data = waveform;
		generator->output_func = wavetable_output;
		generator->mix_func = wavetable_mix_output;
	}
}
