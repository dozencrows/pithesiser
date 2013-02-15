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
#include "system_constants.h"
#include "fixed_point_math.h"
#include "oscillator.h"

#define WAVETABLE_SAMPLE_RATE		((float)SYSTEM_SAMPLE_RATE)
#define WAVETABLE_BASE_FREQUENCY	110.0f

typedef struct
{
	int			sample_count;
	int			frequency;
	fixed_t		phase_limit;
	sample_t	*samples;
	sample_t	*linear_deltas;
} waveform_t;

static int wavetable_initialised = 0;
static waveform_t sine_wave;
static waveform_t saw_wave;
static waveform_t saw_wave_bandlimited;

#define WT_CALC_PHASE_STEP(phase_step, osc, waveform) 	fixed_t phase_step = osc->frequency / (fixed_t)waveform->frequency;

#define WT_GET_SAMPLE(osc, sample) 		   			int wave_index = fixed_round_to_int(osc->phase_accumulator); \
													int32_t sample = waveform->samples[wave_index]

#define WT_LINEAR_INTERP(waveform, osc, sample) 	sample += fixed_mul(waveform->linear_deltas[wave_index], osc->phase_accumulator & FIXED_FRACTION_MASK)

#define WT_ADVANCE_PHASE(osc, waveform, phase_step)	osc->phase_accumulator += phase_step; 					\
													if (osc->phase_accumulator > waveform->phase_limit) 	\
														osc->phase_accumulator -= waveform->phase_limit

static void wavetable_output(waveform_generator_def_t *generator, oscillator_t* osc, sample_t *sample_data, int sample_count)
{
	waveform_t *waveform = (waveform_t*) generator->waveform_data;

	if (waveform != NULL)
	{
		WT_CALC_PHASE_STEP(phase_step, osc, waveform);
		sample_t *sample_ptr = sample_data;
		CALC_AMPLITUDE_INTERPOLATION(osc, amp_scale, amp_delta, sample_count);

		while (sample_count > 0)
		{
			WT_GET_SAMPLE(osc, sample);
			if (generator->flags & GENFLAG_LINEAR_INTERP)
			{
				WT_LINEAR_INTERP(waveform, osc, sample);
			}
			SCALE_AMPLITUDE((amp_scale >> AMPL_INTERP_PRECISION), sample);
			STORE_SAMPLE(sample, sample_ptr);
			WT_ADVANCE_PHASE(osc, waveform, phase_step);
			INTERPOLATE_AMPLITUDE(amp_scale, amp_delta);
			sample_count--;
		}
	}
}

static void wavetable_mix_output(waveform_generator_def_t *generator, oscillator_t* osc, sample_t *sample_data, int sample_count)
{
	waveform_t *waveform = (waveform_t*) generator->waveform_data;

	if (waveform != NULL)
	{
		WT_CALC_PHASE_STEP(phase_step, osc, waveform);
		sample_t *sample_ptr = sample_data;
		CALC_AMPLITUDE_INTERPOLATION(osc, amp_scale, amp_delta, sample_count);

		while (sample_count > 0)
		{
			WT_GET_SAMPLE(osc, sample);
			if (generator->flags & GENFLAG_LINEAR_INTERP)
			{
				WT_LINEAR_INTERP(waveform, osc, sample);
			}
			SCALE_AMPLITUDE((amp_scale >> AMPL_INTERP_PRECISION), sample);
			MIX((int32_t)*sample_ptr, sample, mixed);
			STORE_SAMPLE(mixed, sample_ptr);
			WT_ADVANCE_PHASE(osc, waveform, phase_step);
			INTERPOLATE_AMPLITUDE(amp_scale, amp_delta);
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
		waveform->samples[i] = roundf(sinf(phase) * SHRT_MAX);
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
		waveform->samples[i] = roundf((1.0f - phase * 2.0f) * SHRT_MAX);
	}

	generate_deltas(waveform);
}

static void generate_saw_bandlimited(waveform_t *waveform, float sample_rate, float frequency)
{
	int partials = (WAVETABLE_SAMPLE_RATE / 2) / frequency;

	waveform->frequency = frequency;
	waveform->sample_count = roundf(sample_rate / frequency);
	waveform->phase_limit = waveform->sample_count << FIXED_PRECISION;
	waveform->samples = malloc(waveform->sample_count * sizeof(waveform->samples[0]));

	float sample_max = 0.0f;
	float* sample_buffer = (float*)alloca(waveform->sample_count * sizeof(float));
	float gibbs_constant = M_PI / (2 * (float)partials);

	for (int i = 0; i < waveform->sample_count; i++)
	{
		float sample = 0.0f;
		float phase = 2.0f * M_PI * (float)i / waveform->sample_count;

		for (int s = 1; s <= partials; s++)
		{
			float gibbs = cos((float)(s-1) * gibbs_constant);
			gibbs *= gibbs;
			sample += gibbs * (1/(float)s) * sin((float)s * phase);
		}

		sample_buffer[i] = sample;
		if (sample > sample_max)
		{
			sample_max = sample;
		}
	}

	float sample_normaliser = 1.0f / sample_max;
	for (int i = 0; i < waveform->sample_count; i++)
	{
		float sample = sample_buffer[i] * sample_normaliser;
		waveform->samples[i] = roundf(sample * SHRT_MAX);
	}

	generate_deltas(waveform);
}

void init_wavetables()
{
	if (!wavetable_initialised)
	{
		generate_sine(&sine_wave, WAVETABLE_SAMPLE_RATE, WAVETABLE_BASE_FREQUENCY);
		generate_saw(&saw_wave, WAVETABLE_SAMPLE_RATE, WAVETABLE_BASE_FREQUENCY);
		generate_saw_bandlimited(&saw_wave_bandlimited, WAVETABLE_SAMPLE_RATE, WAVETABLE_BASE_FREQUENCY * 4);
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

		case WAVETABLE_SAW_LINEAR_BL:
			flags = GENFLAG_LINEAR_INTERP;
			waveform = &saw_wave_bandlimited;
			break;

		case WAVETABLE_SAW_BL:
			waveform = &saw_wave_bandlimited;
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
		generator->mid_func = NULL;
	}
}
