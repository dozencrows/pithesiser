/*
 * waveform_procedural.c
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#include <stdlib.h>
#include <limits.h>
#include "waveform.h"
#include "oscillator.h"

#define SAMPLE_RATE			44100
#define PHASE_
#define PHASE_LIMIT			0x00040000
#define PHASE_HALF_LIMIT	(PHASE_LIMIT >> 1)

// Formulae (input is phase, t):
// 	0  to 2: 1 - (1-t)^2
// 	2> to 4: (1-(t-2))^2 - 1

static void procedural_sine_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	// Note can't have osc frequency gtr than 8191 otherwise overflows
	int32_t	phase_step = (PHASE_LIMIT * osc->frequency) / SAMPLE_RATE;
	int16_t *sample_ptr = (int16_t*) sample_data;

	while (sample_count > 0)
	{
		while (osc->phase_accumulator < PHASE_HALF_LIMIT && sample_count > 0)
		{
			int32_t signal = FIXED_ONE - osc->phase_accumulator;
			signal = ((int64_t)signal * (int64_t)signal) >> FIXED_PRECISION;
			signal = (FIXED_ONE - signal) * SHRT_MAX >> FIXED_PRECISION;
			signal = (signal * osc->level) / SHRT_MAX;
			*sample_ptr++ = (int16_t)signal;
			*sample_ptr++ = (int16_t)signal;
			osc->phase_accumulator += phase_step;
			sample_count--;
		}

		while (osc->phase_accumulator < PHASE_LIMIT && sample_count > 0)
		{
			int32_t signal = FIXED_ONE - (osc->phase_accumulator - PHASE_HALF_LIMIT);
			signal = ((int64_t)signal * (int64_t)signal) >> FIXED_PRECISION;
			signal = (signal - FIXED_ONE) * SHRT_MAX >> FIXED_PRECISION;
			signal = (signal * osc->level) / SHRT_MAX;
			*sample_ptr++ = (int16_t)signal;
			*sample_ptr++ = (int16_t)signal;
			osc->phase_accumulator += phase_step;
			sample_count--;
		}

		if (osc->phase_accumulator >= PHASE_LIMIT)
		{
			osc->phase_accumulator -= PHASE_LIMIT;
		}
	}
}

static void procedural_sine_mix_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	// Note can't have osc frequency gtr than 8191 otherwise overflows
	int32_t	phase_step = (PHASE_LIMIT * osc->frequency) / SAMPLE_RATE;
	int16_t *sample_ptr = (int16_t*) sample_data;

	while (sample_count > 0)
	{
		while (osc->phase_accumulator < PHASE_HALF_LIMIT && sample_count > 0)
		{
			int32_t signal = FIXED_ONE - osc->phase_accumulator;
			signal = ((int64_t)signal * (int64_t)signal) >> FIXED_PRECISION;
			signal = (FIXED_ONE - signal) * SHRT_MAX >> FIXED_PRECISION;
			signal = (signal * osc->level) / SHRT_MAX;

			int32_t original = (int32_t)*sample_ptr;
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
			sample_count--;
		}

		while (osc->phase_accumulator < PHASE_LIMIT && sample_count > 0)
		{
			int32_t signal = FIXED_ONE - (osc->phase_accumulator - PHASE_HALF_LIMIT);
			signal = ((int64_t)signal * (int64_t)signal) >> FIXED_PRECISION;
			signal = (signal - FIXED_ONE) * SHRT_MAX >> FIXED_PRECISION;
			signal = (signal * osc->level) / SHRT_MAX;

			int32_t original = (int32_t)*sample_ptr;
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
			sample_count--;
		}

		if (osc->phase_accumulator >= PHASE_LIMIT)
		{
			osc->phase_accumulator -= PHASE_LIMIT;
		}
	}
}

static void procedural_saw_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	// Note can't have osc frequency gtr than 8191 otherwise overflows
	int32_t	phase_step = (PHASE_LIMIT * osc->frequency) / SAMPLE_RATE;
	int16_t *sample_ptr = (int16_t*) sample_data;

	while (sample_count > 0)
	{
		int32_t signal = FIXED_ONE - (osc->phase_accumulator >> 1);
		signal = (signal * SHRT_MAX) >> FIXED_PRECISION;
		signal = (signal * osc->level) / SHRT_MAX;
		*sample_ptr++ = (int16_t)signal;
		*sample_ptr++ = (int16_t)signal;
		osc->phase_accumulator += phase_step;
		sample_count--;

		if (osc->phase_accumulator >= PHASE_LIMIT)
		{
			osc->phase_accumulator -= PHASE_LIMIT;
		}
	}
}

static void procedural_saw_mix_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	// Note can't have osc frequency gtr than 8191 otherwise overflows
	int32_t	phase_step = (PHASE_LIMIT * osc->frequency) / SAMPLE_RATE;
	int16_t *sample_ptr = (int16_t*) sample_data;

	while (sample_count > 0)
	{
		int32_t signal = FIXED_ONE - (osc->phase_accumulator >> 1);
		signal = (signal * SHRT_MAX) >> FIXED_PRECISION;
		signal = (signal * osc->level) / SHRT_MAX;

		int32_t original = (int32_t)*sample_ptr;
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
		sample_count--;

		if (osc->phase_accumulator >= PHASE_LIMIT)
		{
			osc->phase_accumulator -= PHASE_LIMIT;
		}
	}
}

void init_procedural_generator(waveform_type_t waveform_type, waveform_generator_t *generator)
{
	switch (waveform_type)
	{
		case PROCEDURAL_SINE:
			generator->output_func = procedural_sine_output;
			generator->mix_func = procedural_sine_mix_output;
			break;

		case PROCEDURAL_SAW:
			generator->output_func = procedural_saw_output;
			generator->mix_func = procedural_saw_mix_output;
			break;

		default:
			break;
	}

	generator->definition.flags = GENFLAG_NONE;
	generator->definition.waveform_data = NULL;
}
