/*
 * float_filter.c
 *
 *  Created on: 15 Feb 2013
 *      Author: ntuckett
 */

#include "float_filter.h"
#include <memory.h>
#include <math.h>
#include "system_constants.h"
#include "filter.h"

static void clear_history(float_filter_t *filter)
{
	memset(filter->state.history, 0, sizeof(filter->state.history));
	memset(filter->state.output, 0, sizeof(filter->state.output));
}

void float_filter_init(float_filter_t *filter)
{
	clear_history(filter);
	memset(filter->state.input_coeff, 0, sizeof(filter->state.input_coeff));
	memset(filter->state.output_coeff, 0, sizeof(filter->state.output_coeff));
}

void float_filter_update(float_filter_t *filter)
{
	float frequency = filter->definition.frequency;
	float q = filter->definition.q;

	float w0 = 2.0f * M_PI * frequency / (float)SYSTEM_SAMPLE_RATE;
	float cos_w0	= cosf(w0);
	float sin_w0	= sinf(w0);
	float alpha		= sin_w0 / (q + q);

	float a[3];
	float b[3];

	switch(filter->definition.type)
	{
		case FILTER_LPF:
		{
			float tmp = 1.0f - cos_w0;
			b[0] = tmp * 0.5f;
			b[1] = tmp;
			b[2] = tmp * 0.5f;

			a[0] = 1.0f + alpha;
			a[1] = -2.0f * cos_w0;
			a[2] = 1.0f - alpha;
			break;
		}

		case FILTER_HPF:
		{
			float tmp = 1.0f + cos_w0;
			b[0] = tmp * 0.5f;
			b[1] = -tmp;
			b[2] = tmp * 0.5f;

			a[0] = 1.0f + alpha;
			a[1] = -2.0f * cos_w0;
			a[2] = 1.0f - alpha;
			break;
		}

		default:
		{
			b[0] = a[0] = 1.0f;
			a[1] = a[2] = 0.0f;
			b[1] = b[2] = 0.0f;
			break;
		}
	}

	filter->state.input_coeff[0] = b[0] / a[0];
	filter->state.input_coeff[1] = b[1] / a[0];
	filter->state.input_coeff[2] = b[2] / a[0];

	filter->state.output_coeff[0] = a[1] / a[0];
	filter->state.output_coeff[1] = a[2] / a[0];

	clear_history(filter);
}

void float_filter_apply(float_filter_t *filter, float *sample_data, int sample_count)
{
	if (filter->definition.type != FILTER_PASS)
	{
		for (int i = 0; i < sample_count; i++)
		{
			float sample = *sample_data;
			float new_sample;
			new_sample =  sample * filter->state.input_coeff[0];
			new_sample += filter->state.input_coeff[1] * filter->state.history[0];
			new_sample += filter->state.input_coeff[2] * filter->state.history[1];
			new_sample -= filter->state.output_coeff[0] * filter->state.output[0];
			new_sample -= filter->state.output_coeff[1] * filter->state.output[1];

			filter->state.history[1] = filter->state.history[0];
			filter->state.history[0] = sample;
			filter->state.output[1] = filter->state.output[0];
			filter->state.output[0] = new_sample;

			*sample_data++ = new_sample;
		}
	}
	else
	{
		int last_sample_index = (sample_count - 1) * 2;
		float sample = sample_data[last_sample_index];

		filter->state.history[1] = filter->state.history[0];
		filter->state.history[0] = sample;
		filter->state.output[1] = filter->state.output[0];
		filter->state.output[0] = sample;
	}
}
