/*
 * float_waveform.c
 *
 *  Created on: 17 Feb 2013
 *      Author: ntuckett
 */

#include "float_waveform.h"
#include <math.h>
#include <stdlib.h>

void float_generate_sine(float_waveform_t *waveform, float sample_rate, float frequency)
{
	static float max_phase = 1.0f;

	float phase_step_float = max_phase * frequency / sample_rate;
	waveform->frequency = frequency;
	waveform->phase_step = DOUBLE_TO_FIXED(phase_step_float);
	waveform->sample_count = roundf(max_phase / phase_step_float);
	waveform->samples = malloc(waveform->sample_count * sizeof(waveform->samples[0]));

	float phase;

	for (int i = 0; i < waveform->sample_count; i++)
	{
		phase = i * phase_step_float;
		waveform->samples[i] = sinf(phase * M_PI * 2.0f);
	}
}

void waveform_float_procedural_sine(float_oscillator_t *osc, float *sample_buffer, int sample_count)
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

void waveform_float_procedural_sine_mix(float_oscillator_t *osc, float *sample_buffer, int sample_count)
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
			sample += *sample_buffer;
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
			sample += *sample_buffer;
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

void waveform_float_wavetable_sine(float_waveform_t *waveform, float_oscillator_t *osc, float *sample_buffer, int sample_count)
{
	fixed_t frequency_ratio = DOUBLE_TO_FIXED(osc->frequency / waveform->frequency);
	fixed_t phase_step = fixed_mul(frequency_ratio, waveform->phase_step);

	float amplitude_step = (osc->level - osc->last_level) / sample_count;
	float amplitude_scale = osc->last_level;

	while (sample_count > 0)
	{
		int sample_index = fixed_mul(osc->phase_fixed, waveform->sample_count);
		float sample = waveform->samples[sample_index];
		sample *= amplitude_scale;
		*sample_buffer++ = sample;
		*sample_buffer++ = sample;
		osc->phase_fixed += phase_step;
		if (osc->phase_fixed >= FIXED_ONE)
		{
			osc->phase_fixed -= FIXED_ONE;
		}
		amplitude_scale += amplitude_step;
		sample_count--;
	}
}

void waveform_float_wavetable_sine_mix(float_waveform_t *waveform, float_oscillator_t *osc, float *sample_buffer, int sample_count)
{
	fixed_t frequency_ratio = DOUBLE_TO_FIXED(osc->frequency / waveform->frequency);
	fixed_t phase_step = fixed_mul(frequency_ratio, waveform->phase_step);

	float amplitude_step = (osc->level - osc->last_level) / sample_count;
	float amplitude_scale = osc->last_level;

	while (sample_count > 0)
	{
		int sample_index = fixed_mul(osc->phase_fixed, waveform->sample_count);
		float sample = waveform->samples[sample_index];
		sample *= amplitude_scale;
		sample += *sample_buffer;
		*sample_buffer++ = sample;
		*sample_buffer++ = sample;
		osc->phase_fixed += phase_step;
		if (osc->phase_fixed >= FIXED_ONE)
		{
			osc->phase_fixed -= FIXED_ONE;
		}
		amplitude_scale += amplitude_step;
		sample_count--;
	}

}



