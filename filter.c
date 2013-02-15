/*
 * filter.c
 *
 *  Created on: 3 Feb 2013
 *      Author: ntuckett
 */

#include "filter.h"
#include <memory.h>
#include "fixed_point_math.h"

static void clear_history(filter_t *filter)
{
	memset(filter->state.history, 0, sizeof(filter->state.history));
	memset(filter->state.output, 0, sizeof(filter->state.output));
}

void filter_init(filter_t *filter)
{
	clear_history(filter);
	memset(filter->state.input_coeff, 0, sizeof(filter->state.input_coeff));
	memset(filter->state.output_coeff, 0, sizeof(filter->state.output_coeff));
}

void filter_update(filter_t *filter)
{
	fixed_wide_t w0 = fixed_mul_wide_start(FIXED_PI, filter->definition.frequency);
	w0 = (w0 + w0) / (fixed_wide_t)SYSTEM_SAMPLE_RATE;
	fixed_t cos_w0, sin_w0;

	fixed_sin_cos((fixed_t)w0, &sin_w0, &cos_w0);

	fixed_t alpha	= fixed_divide(sin_w0, filter->definition.q * 2);

	fixed_wide_t a[3];
	fixed_wide_t b[3];

	switch(filter->definition.type)
	{
		case FILTER_LPF:
		{
			fixed_wide_t tmp = FIXED_ONE - cos_w0;
			b[0] = tmp >> 1;
			b[1] = tmp;
			b[2] = tmp >> 1;

			a[0] = FIXED_ONE + alpha;
			a[1] = -2 * (fixed_wide_t) cos_w0;
			a[2] = FIXED_ONE - alpha;
			break;
		}

		case FILTER_HPF:
		{
			fixed_wide_t tmp = FIXED_ONE + cos_w0;
			b[0] = tmp >> 1;
			b[1] = -tmp;
			b[2] = tmp >> 1;

			a[0] = FIXED_ONE + alpha;
			a[1] = -2 * (fixed_wide_t) cos_w0;
			a[2] = FIXED_ONE - alpha;
			break;
		}

		default:
		{
			b[0] = a[0] = FIXED_ONE;
			a[1] = a[2] = 0;
			b[1] = b[2] = 0;
			break;
		}
	}

	filter->state.input_coeff[0] = fixed_divide_wide(b[0], a[0]);
	filter->state.input_coeff[1] = fixed_divide_wide(b[1], a[0]);
	filter->state.input_coeff[2] = fixed_divide_wide(b[2], a[0]);

	filter->state.output_coeff[0] = fixed_divide_wide(a[1], a[0]);
	filter->state.output_coeff[1] = fixed_divide_wide(a[2], a[0]);

	clear_history(filter);
}

void filter_apply(filter_t *filter, sample_t *sample_data, int sample_count)
{
	if (filter->definition.type != FILTER_PASS)
	{
		for (int i = 0; i < sample_count; i++)
		{
			fixed_wide_t sample = (fixed_wide_t)*sample_data;
			fixed_wide_t new_sample;
			new_sample =  sample * filter->state.input_coeff[0];
			new_sample += fixed_mul_wide(filter->state.input_coeff[1], filter->state.history[0]);
			new_sample += fixed_mul_wide(filter->state.input_coeff[2], filter->state.history[1]);
			new_sample -= fixed_mul_wide(filter->state.output_coeff[0], filter->state.output[0]);
			new_sample -= fixed_mul_wide(filter->state.output_coeff[1], filter->state.output[1]);

			filter->state.history[1] = filter->state.history[0];
			filter->state.history[0] = sample << FIXED_PRECISION;
			filter->state.output[1] = filter->state.output[0];
			filter->state.output[0] = new_sample;

			sample_t output = (sample_t)fixed_wide_round_to_int(new_sample);
			*sample_data++ = output;
			*sample_data++ = output;
		}
	}
	else
	{
		int last_sample_index = (sample_count - 1) * 2;
		fixed_wide_t sample = fixed_wide_from_int(sample_data[last_sample_index]);

		filter->state.history[1] = filter->state.history[0];
		filter->state.history[0] = sample;
		filter->state.output[1] = filter->state.output[0];
		filter->state.output[0] = sample;
	}
}
