/*
 * oscillator.c
 *
 *  Created on: 31 Oct 2012
 *      Author: ntuckett
 *
 * Ideas to try:
 * 	* Phil Atkin's sine approximation - not as a table lookup
 * 	* Cubic interpolation with table lookup
 * 	* Phase Distortion control (simple linear)
 * 	* Better framework for flexible oscillator construction/configuration
 *    Ideas:
 *      * Function ptrs: e.g. for wavetable vs calculated forms
 *      * Macros for common operations: e.g. phase accumulator reading, wavetable lookup, phase accumulator update, mixing
 *      * "Bulk" evaluation entrypoint for a set of voices; handles mixing automatically
 */
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "oscillator.h"

#define WAVETABLE_SAMPLE_RATE		44100.0f
#define WAVETABLE_BASE_FREQUENCY	110.25f

#define FIXED_PRECISION				16
#define FIXED_FRACTION_MASK			0x0000ffff

#define GENFLAG_NONE				0x00000000
#define GENFLAG_LINEAR_INTERP		0x00000001

typedef struct
{
	int			sample_count;
	int			frequency;
	u_int32_t	phase_limit;
	int16_t		*samples;
	int16_t		*linear_deltas;
} waveform_t;

typedef struct
{
	waveform_t				*waveform;
	u_int32_t				flags;
} waveform_generator_def_t;

typedef void (*generator_output_func_t)(waveform_generator_def_t *generator_def, oscillator_t* osc, int sample_count, void *sample_data);

typedef struct
{
	waveform_generator_def_t	definition;
	generator_output_func_t		output_func;
	generator_output_func_t		mix_func;
} waveform_generator_t;

static int wavetable_initialised = 0;
static waveform_t sine_wave;
static waveform_t saw_wave;

static void wavetable_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	waveform_t *waveform = generator->waveform;

	if (waveform != NULL)
	{
		u_int32_t 	phase_step = ((u_int32_t)osc->frequency << FIXED_PRECISION) / (u_int32_t) waveform->frequency;
		int16_t 	*sample_ptr = (int16_t*) sample_data;

		while (sample_count > 0)
		{
			u_int32_t wave_index = osc->phase_accumulator >> FIXED_PRECISION;
			int32_t signal = waveform->samples[wave_index];
			if (generator->flags & GENFLAG_LINEAR_INTERP)
			{
				signal += (((int32_t) waveform->linear_deltas[wave_index]) * ((int32_t)osc->phase_accumulator & FIXED_FRACTION_MASK)) >> FIXED_PRECISION;
			}
			signal = (signal * osc->level) / SHRT_MAX;
			*sample_ptr++ = (int16_t)signal;
			*sample_ptr++ = (int16_t)signal;
			osc->phase_accumulator += phase_step;
			if (osc->phase_accumulator > waveform->phase_limit)
			{
				osc->phase_accumulator -= waveform->phase_limit;
			}

			sample_count--;
		}
	}
}

static void wavetable_mix_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	waveform_t *waveform = generator->waveform;

	if (waveform != NULL)
	{
		u_int32_t 	phase_step = ((u_int32_t)osc->frequency << FIXED_PRECISION) / (u_int32_t) waveform->frequency;
		int16_t 	*sample_ptr = (int16_t*) sample_data;

		while (sample_count > 0)
		{
			u_int32_t wave_index = osc->phase_accumulator >> FIXED_PRECISION;
			int32_t original = (int32_t)*sample_ptr;
			int32_t signal = waveform->samples[wave_index];
			if (generator->flags & GENFLAG_LINEAR_INTERP)
			{
				signal += (((int32_t) waveform->linear_deltas[wave_index]) * ((int32_t)osc->phase_accumulator & FIXED_FRACTION_MASK)) >> FIXED_PRECISION;
			}
			signal = (signal * osc->level) / SHRT_MAX;
			int32_t mixed_sample = original + signal;

			mixed_sample = (mixed_sample * 75) / 100;
			if (mixed_sample < -SHRT_MAX)
			{
				mixed_sample = -SHRT_MAX;
			}
			else if (mixed_sample > SHRT_MAX)
			{
				mixed_sample = SHRT_MAX;
			}

			*sample_ptr++ = (int16_t)mixed_sample;
			*sample_ptr++ = (int16_t)mixed_sample;
			osc->phase_accumulator += phase_step;
			if (osc->phase_accumulator > waveform->phase_limit)
			{
				osc->phase_accumulator -= waveform->phase_limit;
			}

			sample_count--;
		}
	}
}

static waveform_generator_t generators[] =
{
	{ { &sine_wave, GENFLAG_NONE }, wavetable_output, wavetable_mix_output },
	{ { &saw_wave, 	GENFLAG_NONE }, wavetable_output, wavetable_mix_output },
	{ { &sine_wave, GENFLAG_LINEAR_INTERP }, wavetable_output, wavetable_mix_output },
	{ { &saw_wave, 	GENFLAG_LINEAR_INTERP }, wavetable_output, wavetable_mix_output },
};

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
	waveform->sample_count = max_phase / phase_step;
	waveform->phase_limit = (u_int32_t)waveform->sample_count << FIXED_PRECISION;
	waveform->samples = malloc(waveform->sample_count * sizeof(waveform->samples[0]));

	for (int i = 0; i < waveform->sample_count; i++)
	{
		float phase = i * phase_step;
		waveform->samples[i] = sinf(phase) * SHRT_MAX;
	}

	generate_deltas(waveform);
}

static void generate_saw(waveform_t *waveform, float sample_rate, float frequency)
{
	float phase_step = frequency / sample_rate;

	waveform->frequency = frequency;
	waveform->sample_count = 1.0f / phase_step;
	waveform->phase_limit = (u_int32_t)waveform->sample_count << FIXED_PRECISION;
	waveform->samples = malloc(waveform->sample_count * sizeof(waveform->samples[0]));

	for (int i = 0; i < waveform->sample_count; i++)
	{
		float phase = i * phase_step;
		waveform->samples[i] = (1.0f - phase * 2.0f) * SHRT_MAX;
	}

	generate_deltas(waveform);
}

void osc_init(oscillator_t* osc)
{
	if (!wavetable_initialised)
	{
		generate_sine(&sine_wave, WAVETABLE_SAMPLE_RATE, WAVETABLE_BASE_FREQUENCY);
		generate_saw(&saw_wave, WAVETABLE_SAMPLE_RATE, WAVETABLE_BASE_FREQUENCY);
		wavetable_initialised = 1;
	}

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
