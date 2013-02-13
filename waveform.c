/*
 * waveform.c
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#include "waveform.h"
#include "waveform_internal.h"
#include "waveform_wavetable.h"
#include "waveform_procedural.h"

waveform_generator_t generators[WAVE_COUNT];

void waveform_initialise()
{
	init_wavetables();
	init_wavetable_generator(WAVETABLE_SINE, &generators[WAVETABLE_SINE]);
	init_wavetable_generator(WAVETABLE_SAW, &generators[WAVETABLE_SAW]);
	init_wavetable_generator(WAVETABLE_SAW_BL, &generators[WAVETABLE_SAW_BL]);
	init_wavetable_generator(WAVETABLE_SINE_LINEAR, &generators[WAVETABLE_SINE_LINEAR]);
	init_wavetable_generator(WAVETABLE_SAW_LINEAR, &generators[WAVETABLE_SAW_LINEAR]);
	init_wavetable_generator(WAVETABLE_SAW_LINEAR_BL, &generators[WAVETABLE_SAW_LINEAR_BL]);

	init_procedural_generator(PROCEDURAL_SINE, &generators[PROCEDURAL_SINE]);
	init_procedural_generator(PROCEDURAL_SAW, &generators[PROCEDURAL_SAW]);
	init_procedural_generator(LFO_PROCEDURAL_SINE, &generators[LFO_PROCEDURAL_SINE]);
	init_procedural_generator(LFO_PROCEDURAL_SAW_DOWN, &generators[LFO_PROCEDURAL_SAW_DOWN]);
	init_procedural_generator(LFO_PROCEDURAL_SAW_UP, &generators[LFO_PROCEDURAL_SAW_UP]);
	init_procedural_generator(LFO_PROCEDURAL_TRIANGLE, &generators[LFO_PROCEDURAL_TRIANGLE]);
	init_procedural_generator(LFO_PROCEDURAL_SQUARE, &generators[LFO_PROCEDURAL_SQUARE]);
	init_procedural_generator(LFO_PROCEDURAL_HALFSAW_DOWN, &generators[LFO_PROCEDURAL_HALFSAW_DOWN]);
	init_procedural_generator(LFO_PROCEDURAL_HALFSAW_UP, &generators[LFO_PROCEDURAL_HALFSAW_UP]);
	init_procedural_generator(LFO_PROCEDURAL_HALFSINE, &generators[LFO_PROCEDURAL_HALFSINE]);
	init_procedural_generator(LFO_PROCEDURAL_HALFTRIANGLE, &generators[LFO_PROCEDURAL_HALFTRIANGLE]);
}

// Using 16.15 fixed point
#define SCALE_PRECISION	15
#define SCALE_ONE		(1 << SCALE_PRECISION)

static int32_t note_scale_factors[] = { 32768, 24576, 18432, 13824, 10368, 7776, 5832, 4374 };

void waveform_normalise(sample_t *buffer, int sample_count, int last_note_count, int current_note_count)
{
	int32_t start_scale = note_scale_factors[last_note_count - 1];

	if (last_note_count != current_note_count)
	{
		int32_t end_scale = note_scale_factors[current_note_count - 1];
		int32_t scale_delta = (end_scale - start_scale) / sample_count;
		int32_t scale = start_scale;

		sample_t *sample_ptr = buffer;
		for (int i = 0; i < sample_count; i++)
		{
			int32_t sample = *sample_ptr;
			sample = (sample * scale) >> SCALE_PRECISION;
			if (sample > SAMPLE_MAX)
			{
				sample = SAMPLE_MAX;
			}
			else if (sample < -SAMPLE_MAX)
			{
				sample = -SAMPLE_MAX;
			}
			*sample_ptr++ = (sample_t)sample;
			*sample_ptr++ = (sample_t)sample;
			scale += scale_delta;
		}
	}
	else if (last_note_count > 1)
	{
		int32_t scale = start_scale;
		sample_t *sample_ptr = buffer;
		for (int i = 0; i < sample_count; i++)
		{
			int32_t sample = *sample_ptr;
			sample = (sample * scale) >> SCALE_PRECISION;
			if (sample > SAMPLE_MAX)
			{
				sample = SAMPLE_MAX;
			}
			else if (sample < -SAMPLE_MAX)
			{
				sample = -SAMPLE_MAX;
			}
			*sample_ptr++ = (sample_t)sample;
			*sample_ptr++ = (sample_t)sample;
		}
	}
}
