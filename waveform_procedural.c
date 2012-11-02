/*
 * waveform_procedural.c
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#include <stdlib.h>
#include <limits.h>
#include "waveform_procedural.h"
#include "oscillator.h"

#define SAMPLE_RATE			44100
#define PHASE_
#define PHASE_LIMIT			0x00040000
#define PHASE_HALF_LIMIT	(PHASE_LIMIT >> 1)

// Note can't have osc frequency gtr than 8191 otherwise overflows
#define PR_CALC_PHASE_STEP(osc, phase_step)	int32_t	phase_step = (PHASE_LIMIT * osc->frequency) / SAMPLE_RATE

// Formulae (input is phase, t):
// 	0  to 2: 1 - (1-t)^2
// 	2> to 4: (1-(t-2))^2 - 1
#define PR_CALC_SINE_POSITIVE(osc, sample)	int32_t sample = FIXED_ONE - osc->phase_accumulator; \
											sample = ((int64_t)sample * (int64_t)sample) >> FIXED_PRECISION; \
											sample = (FIXED_ONE - sample) * SHRT_MAX >> FIXED_PRECISION

#define PR_CALC_SINE_NEGATIVE(osc, sample)	int32_t sample = FIXED_ONE - (osc->phase_accumulator - PHASE_HALF_LIMIT); \
											sample = ((int64_t)sample * (int64_t)sample) >> FIXED_PRECISION; \
											sample = (sample - FIXED_ONE) * SHRT_MAX >> FIXED_PRECISION

#define PR_CALC_SAW(osc, sample)			int32_t sample = FIXED_ONE - (osc->phase_accumulator >> 1);	\
											sample = (sample * SHRT_MAX) >> FIXED_PRECISION;

#define PR_ADVANCE_PHASE(osc, phase_step)	osc->phase_accumulator += phase_step

#define PR_LOOP_PHASE(osc)					if (osc->phase_accumulator >= PHASE_LIMIT) \
												osc->phase_accumulator -= PHASE_LIMIT

static void procedural_sine_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	PR_CALC_PHASE_STEP(osc, phase_step);
	int16_t *sample_ptr = (int16_t*) sample_data;

	while (sample_count > 0)
	{
		while (osc->phase_accumulator < PHASE_HALF_LIMIT && sample_count > 0)
		{
			PR_CALC_SINE_POSITIVE(osc, sample);
			SCALE_AMPLITUDE(osc, sample)
			STORE_SAMPLE(sample, sample_ptr);
			PR_ADVANCE_PHASE(osc, phase_step);
			sample_count--;
		}

		while (osc->phase_accumulator < PHASE_LIMIT && sample_count > 0)
		{
			PR_CALC_SINE_NEGATIVE(osc, sample);
			SCALE_AMPLITUDE(osc, sample)
			STORE_SAMPLE(sample, sample_ptr);
			PR_ADVANCE_PHASE(osc, phase_step);
			sample_count--;
		}

		PR_LOOP_PHASE(osc);
	}
}

static void procedural_sine_mix_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	PR_CALC_PHASE_STEP(osc, phase_step);
	int16_t *sample_ptr = (int16_t*) sample_data;

	while (sample_count > 0)
	{
		while (osc->phase_accumulator < PHASE_HALF_LIMIT && sample_count > 0)
		{
			PR_CALC_SINE_POSITIVE(osc, sample);
			SCALE_AMPLITUDE(osc, sample)
			MIX((int32_t)*sample_ptr, sample, mixed);
			STORE_SAMPLE(mixed, sample_ptr);
			PR_ADVANCE_PHASE(osc, phase_step);
			sample_count--;
		}

		while (osc->phase_accumulator < PHASE_LIMIT && sample_count > 0)
		{
			PR_CALC_SINE_NEGATIVE(osc, sample);
			SCALE_AMPLITUDE(osc, sample)
			MIX((int32_t)*sample_ptr, sample, mixed);
			STORE_SAMPLE(mixed, sample_ptr);
			PR_ADVANCE_PHASE(osc, phase_step);
			sample_count--;
		}

		PR_LOOP_PHASE(osc);
	}
}

static void procedural_saw_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	PR_CALC_PHASE_STEP(osc, phase_step);
	int16_t *sample_ptr = (int16_t*) sample_data;

	while (sample_count > 0)
	{
		PR_CALC_SAW(osc, sample);
		SCALE_AMPLITUDE(osc, sample);
		STORE_SAMPLE(sample, sample_ptr);
		PR_ADVANCE_PHASE(osc, phase_step);
		sample_count--;

		PR_LOOP_PHASE(osc);
	}
}

static void procedural_saw_mix_output(waveform_generator_def_t *generator, oscillator_t* osc, int sample_count, void *sample_data)
{
	PR_CALC_PHASE_STEP(osc, phase_step);
	int16_t *sample_ptr = (int16_t*) sample_data;

	while (sample_count > 0)
	{
		PR_CALC_SAW(osc, sample);
		SCALE_AMPLITUDE(osc, sample);
		MIX((int32_t)*sample_ptr, sample, mixed);
		STORE_SAMPLE(mixed, sample_ptr);
		PR_ADVANCE_PHASE(osc, phase_step);
		sample_count--;

		PR_LOOP_PHASE(osc);
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
