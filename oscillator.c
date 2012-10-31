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

#define WAVETABLE_SAMPLE_RATE		44100.0f
#define WAVETABLE_BASE_FREQUENCY	110.25f

#define FIXED_PRECISION				16

typedef struct
{
	int			sample_count;
	float		frequency;
	u_int32_t	phase_limit;
	int16_t		*samples;
} waveform_t;

static int wavetable_initialised = 0;
static waveform_t sine_wave;
static waveform_t saw_wave;

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
}

void osc_init(oscillator_t* osc)
{
	if (!wavetable_initialised)
	{
		generate_sine(&sine_wave, WAVETABLE_SAMPLE_RATE, WAVETABLE_BASE_FREQUENCY);
		generate_saw(&saw_wave, WAVETABLE_SAMPLE_RATE, WAVETABLE_BASE_FREQUENCY);
		wavetable_initialised = 1;
	}

	osc->waveform 	= WAVE_SINE;
	osc->frequency	= 440.0f;
}

void osc_output(oscillator_t* osc, int sample_count, void *sample_data)
{
	waveform_t *waveform = NULL;

	switch (osc->waveform)
	{
		case WAVE_SINE:
			waveform = &sine_wave;
			break;

		case WAVE_SAW:
			waveform = &saw_wave;
			break;
	}

	if (waveform != NULL)
	{
		u_int32_t 	phase_step = ((u_int32_t)osc->frequency << FIXED_PRECISION) / (u_int32_t) waveform->frequency;

		int16_t *sample_ptr = (int16_t*) sample_data;

		while (sample_count > 0)
		{
			u_int32_t wave_index = osc->phase_accumulator >> FIXED_PRECISION;
			*sample_ptr++ = waveform->samples[wave_index];
			*sample_ptr++ = waveform->samples[wave_index];
			osc->phase_accumulator += phase_step;
			if (osc->phase_accumulator > waveform->phase_limit)
			{
				osc->phase_accumulator -= waveform->phase_limit;
			}

			sample_count--;
		}
	}
}
