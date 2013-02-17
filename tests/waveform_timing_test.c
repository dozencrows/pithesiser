/*
 * float_waveform_timing_test.c
 *
 *  Created on: 16 Feb 2013
 *      Author: ntuckett
 */

#include <stdio.h>
#include <stdlib.h>
#include "../system_constants.h"
#include "../master_time.h"
#include "../waveform.h"
#include "../oscillator.h"
#include "../waveform_internal.h"
#include "../float_waveform.h"
#include "../fixed_point_math.h"

extern waveform_generator_t generators[];

static const int TEST_BUFFER_SIZE = 128;

static void waveform_float_procedural_sine(float_oscillator_t *osc, float *sample_buffer, int sample_count)
{
	float phase_step = osc->frequency / SYSTEM_SAMPLE_RATE;
	float amplitude_step = (osc->level - osc->last_level) / sample_count;
	float amplitude_scale = osc->last_level;

	while (sample_count > 0)
	{
		while (osc->phase < 0.5f && sample_count > 0)
		{
			float sample = 1.0f - osc->phase;
			sample = 1.0f - (sample * sample);
			sample *= amplitude_scale;
			*sample_buffer++ = sample;
			*sample_buffer++ = sample;
			osc->phase += phase_step;
			amplitude_scale += amplitude_step;
			sample_count--;
		}

		while (osc->phase < 1.0f && sample_count > 0)
		{
			float sample = 1.0f - (osc->phase - 0.5f);
			sample = (sample * sample) - 1.0f;
			sample *= amplitude_scale;
			*sample_buffer++ = sample;
			*sample_buffer++ = sample;
			osc->phase += phase_step;
			amplitude_scale += amplitude_step;
			sample_count--;
		}

		if (osc->phase >= 1.0f)
		{
			osc->phase -= 1.0f;
		}
	}
}

//#define PHASE_LIMIT			(4 * FIXED_ONE)
//#define PHASE_HALF_LIMIT	(2 * FIXED_ONE)
//#define PHASE_QUARTER_LIMIT	(FIXED_ONE)
//
//#define PR_CALC_PHASE_STEP(osc, phase_step)	fixed_t	phase_step = fixed_mul(PHASE_LIMIT, osc->frequency) / SYSTEM_SAMPLE_RATE
//#define PR_CALC_SINE_POSITIVE(osc, sample)	sample = FIXED_ONE - osc->phase_accumulator; \
//											sample = fixed_mul(sample, sample); \
//											sample = fixed_mul(FIXED_ONE - sample, SAMPLE_MAX)
//
//#define PR_CALC_SINE_NEGATIVE(osc, sample)	sample = FIXED_ONE - (osc->phase_accumulator - PHASE_HALF_LIMIT); \
//											sample = fixed_mul(sample, sample); \
//											sample = fixed_mul(sample - FIXED_ONE, SAMPLE_MAX)
//
//#define PR_ADVANCE_PHASE(osc, phase_step)	osc->phase_accumulator += phase_step
//
//#define PR_LOOP_PHASE(osc)					if (osc->phase_accumulator >= PHASE_LIMIT) \
//												osc->phase_accumulator -= PHASE_LIMIT
//
//static void procedural_sine_output(waveform_generator_def_t *generator, oscillator_t* osc, sample_t *sample_data, int sample_count)
//{
//	PR_CALC_PHASE_STEP(osc, phase_step);
//	sample_t *sample_ptr = sample_data;
//	CALC_AMPLITUDE_INTERPOLATION(osc, amp_scale, amp_delta, sample_count);
//
//	while (sample_count > 0)
//	{
//		while (osc->phase_accumulator < PHASE_HALF_LIMIT && sample_count > 0)
//		{
//			int32_t PR_CALC_SINE_POSITIVE(osc, sample);
//			SCALE_AMPLITUDE((amp_scale >> AMPL_INTERP_PRECISION), sample);
//			STORE_SAMPLE(sample, sample_ptr);
//			PR_ADVANCE_PHASE(osc, phase_step);
//			INTERPOLATE_AMPLITUDE(amp_scale, amp_delta);
//			sample_count--;
//		}
//
//		while (osc->phase_accumulator < PHASE_LIMIT && sample_count > 0)
//		{
//			int32_t PR_CALC_SINE_NEGATIVE(osc, sample);
//			SCALE_AMPLITUDE((amp_scale >> AMPL_INTERP_PRECISION), sample);
//			STORE_SAMPLE(sample, sample_ptr);
//			PR_ADVANCE_PHASE(osc, phase_step);
//			INTERPOLATE_AMPLITUDE(amp_scale, amp_delta);
//			sample_count--;
//		}
//
//		PR_LOOP_PHASE(osc);
//	}
//}

static void procedural_sine_timing_test(int count)
{
	float_oscillator_t	float_osc;
	float				float_sample_buffer[TEST_BUFFER_SIZE * 2];
	oscillator_t		fixed_osc;
	sample_t			int_sample_buffer[TEST_BUFFER_SIZE * 2];

	osc_init(&fixed_osc);
	fixed_osc.waveform = PROCEDURAL_SINE;
	fixed_osc.level = LEVEL_MAX;
	fixed_osc.last_level = LEVEL_MAX;

	float_osc.frequency = 440.0f;
	float_osc.level = 1.0f;
	float_osc.last_level = 1.0f;
	float_osc.phase = 0.0f;

	generator_output_func_t waveform_fixed_procedural_sine = generators[PROCEDURAL_SINE].output_func;

	int32_t start_fixed_time = get_elapsed_time_ms();
	for (int i = 0; i < count; i++)
	{
		waveform_fixed_procedural_sine(&generators[PROCEDURAL_SINE].definition, &fixed_osc, int_sample_buffer, TEST_BUFFER_SIZE);
		//procedural_sine_output(&generators[PROCEDURAL_SINE].definition, &fixed_osc, int_sample_buffer, TEST_BUFFER_SIZE);
		fixed_osc.last_level = fixed_osc.level;
	}

	int32_t start_float_time = get_elapsed_time_ms();
	for (int i = 0; i < count; i++)
	{
		waveform_float_procedural_sine(&float_osc, float_sample_buffer, TEST_BUFFER_SIZE);
		float_osc.last_level = float_osc.level;
	}

	int32_t end_time = get_elapsed_time_ms();

	printf("Procedural sine output: for %d iterations: fixed = %dms, float = %dms\n", count, start_float_time - start_fixed_time, end_time - start_float_time);

	waveform_fixed_procedural_sine = generators[PROCEDURAL_SINE].mix_func;

	start_fixed_time = get_elapsed_time_ms();
	for (int i = 0; i < count; i++)
	{
		waveform_fixed_procedural_sine(&generators[PROCEDURAL_SINE].definition, &fixed_osc, int_sample_buffer, TEST_BUFFER_SIZE);
		fixed_osc.last_level = fixed_osc.level;
	}

	start_float_time = get_elapsed_time_ms();
	for (int i = 0; i < count; i++)
	{
		waveform_float_procedural_sine_mix(&float_osc, float_sample_buffer, TEST_BUFFER_SIZE);
		float_osc.last_level = float_osc.level;
	}

	end_time = get_elapsed_time_ms();

	printf("Procedural sine mix: for %d iterations: fixed = %dms, float = %dms\n", count, start_float_time - start_fixed_time, end_time - start_float_time);
}

static void wavetable_sine_timing_test(int count)
{
	float_oscillator_t	float_osc;
	float				float_sample_buffer[TEST_BUFFER_SIZE * 2];
	float_waveform_t	float_waveform;
	oscillator_t		fixed_osc;
	sample_t			int_sample_buffer[TEST_BUFFER_SIZE * 2];

	osc_init(&fixed_osc);
	fixed_osc.waveform = WAVETABLE_SINE;
	fixed_osc.level = LEVEL_MAX;
	fixed_osc.last_level = LEVEL_MAX;

	float_generate_sine(&float_waveform, SYSTEM_SAMPLE_RATE, 110.0f);
	float_osc.frequency = 440.0f;
	float_osc.level = 1.0f;
	float_osc.last_level = 1.0f;
	float_osc.phase_fixed = 0;

	generator_output_func_t waveform_fixed_wavetable_sine = generators[WAVETABLE_SINE].output_func;

	int32_t start_fixed_time = get_elapsed_time_ms();
	for (int i = 0; i < count; i++)
	{
		waveform_fixed_wavetable_sine(&generators[WAVETABLE_SINE].definition, &fixed_osc, int_sample_buffer, TEST_BUFFER_SIZE);
		fixed_osc.last_level = fixed_osc.level;
	}

	int32_t start_float_time = get_elapsed_time_ms();
	for (int i = 0; i < count; i++)
	{
		waveform_float_wavetable_sine(&float_waveform, &float_osc, float_sample_buffer, TEST_BUFFER_SIZE);
		float_osc.last_level = float_osc.level;
	}

	int32_t end_time = get_elapsed_time_ms();

	printf("Wavetable sine output: for %d iterations: fixed = %dms, float = %dms\n", count, start_float_time - start_fixed_time, end_time - start_float_time);

	waveform_fixed_wavetable_sine = generators[WAVETABLE_SINE].mix_func;

	start_fixed_time = get_elapsed_time_ms();
	for (int i = 0; i < count; i++)
	{
		waveform_fixed_wavetable_sine(&generators[WAVETABLE_SINE].definition, &fixed_osc, int_sample_buffer, TEST_BUFFER_SIZE);
		fixed_osc.last_level = fixed_osc.level;
	}

	start_float_time = get_elapsed_time_ms();
	for (int i = 0; i < count; i++)
	{
		waveform_float_wavetable_sine_mix(&float_waveform, &float_osc, float_sample_buffer, TEST_BUFFER_SIZE);
		float_osc.last_level = float_osc.level;
	}

	end_time = get_elapsed_time_ms();

	printf("Wavetable sine mix: for %d iterations: fixed = %dms, float = %dms\n", count, start_float_time - start_fixed_time, end_time - start_float_time);
}

void waveform_timing_test(int count)
{
	printf("*********************************************************************\n");
	printf("Waveform Timing Test\n");

	waveform_initialise();
	procedural_sine_timing_test(count);
	wavetable_sine_timing_test(count);
}
