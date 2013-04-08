/*
 * mixer.c
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#include "mixer.h"

void copy_mono_to_stereo(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest)
{
	for (int i = 0; i < sample_count; i++)
	{
		int32_t original_sample = *source;

		int32_t mixed_sample = (original_sample * left) / PAN_MAX;
		*dest++ = mixed_sample;

		mixed_sample = (original_sample * right) / PAN_MAX;
		*dest++ = mixed_sample;
		source++;
	}
}

void mixdown_mono_to_stereo(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest)
{
	for (int i = 0; i < sample_count; i++)
	{
		int32_t original_sample = *source;

		int32_t mixed_sample = (original_sample * left) / PAN_MAX + *dest;

		if (mixed_sample < -SAMPLE_MAX)
		{
			mixed_sample = -SAMPLE_MAX;
		}
		else if (mixed_sample > SAMPLE_MAX)
		{
			mixed_sample = SAMPLE_MAX;
		}
		*dest++ = mixed_sample;

		mixed_sample = (original_sample * right) / PAN_MAX + *dest;

		if (mixed_sample < -SAMPLE_MAX)
		{
			mixed_sample = -SAMPLE_MAX;
		}
		else if (mixed_sample > SAMPLE_MAX)
		{
			mixed_sample = SAMPLE_MAX;
		}
		*dest++ = mixed_sample;
		source++;
	}
}
