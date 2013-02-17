/*
 * filter.c
 *
 *  Created on: 3 Feb 2013
 *      Author: ntuckett
 */

#include "filter.h"
#include <memory.h>
#include "fixed_point_math.h"

#define FILTER_PRECISION_DELTA  (FIXED_PRECISION - FILTER_FIXED_PRECISION)

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
	fixed_wide_t w0 = fixed_mul_wide(FIXED_PI, ((fixed_wide_t)filter->definition.frequency) << FILTER_PRECISION_DELTA);
	w0 = (w0 + w0) / SYSTEM_SAMPLE_RATE;
	fixed_t cos_w0, sin_w0;

	fixed_sin_cos((fixed_t)w0, &sin_w0, &cos_w0);

	sin_w0 >>= FILTER_PRECISION_DELTA;
	cos_w0 >>= FILTER_PRECISION_DELTA;

	fixed_t q = filter->definition.q >> FILTER_PRECISION_DELTA;
	fixed_t alpha	= fixed_divide_at(sin_w0, q * 2, FILTER_FIXED_PRECISION);

	fixed_t a[3];
	fixed_t b[3];

	switch(filter->definition.type)
	{
		case FILTER_LPF:
		{
			fixed_t tmp = FILTER_FIXED_ONE - cos_w0;
			b[0] = tmp >> 1;
			b[1] = tmp;
			b[2] = tmp >> 1;

			a[0] = FILTER_FIXED_ONE + alpha;
			a[1] = -2 * cos_w0;
			a[2] = FILTER_FIXED_ONE - alpha;
			break;
		}

		case FILTER_HPF:
		{
			fixed_t tmp = FILTER_FIXED_ONE + cos_w0;
			b[0] = tmp >> 1;
			b[1] = -tmp;
			b[2] = tmp >> 1;

			a[0] = FILTER_FIXED_ONE + alpha;
			a[1] = -2 * cos_w0;
			a[2] = FILTER_FIXED_ONE - alpha;
			break;
		}

		default:
		{
			b[0] = a[0] = FILTER_FIXED_ONE;
			a[1] = a[2] = 0;
			b[1] = b[2] = 0;
			break;
		}
	}

	filter->state.input_coeff[0] = fixed_divide_at(b[0], a[0], FILTER_FIXED_PRECISION);
	filter->state.input_coeff[1] = fixed_divide_at(b[1], a[0], FILTER_FIXED_PRECISION);
	filter->state.input_coeff[2] = fixed_divide_at(b[2], a[0], FILTER_FIXED_PRECISION);

	filter->state.output_coeff[0] = fixed_divide_at(a[1], a[0], FILTER_FIXED_PRECISION);
	filter->state.output_coeff[1] = fixed_divide_at(a[2], a[0], FILTER_FIXED_PRECISION);

	clear_history(filter);
}

void filter_apply(filter_t *filter, sample_t *sample_data, int sample_count)
{
	if (filter->definition.type != FILTER_PASS)
	{
		for (int i = 0; i < sample_count; i++)
		{
			fixed_t sample = (fixed_t)*sample_data;
			fixed_t new_sample;
			new_sample =  sample * filter->state.input_coeff[0];
			new_sample += fixed_mul_at(filter->state.input_coeff[1], filter->state.history[0], FILTER_FIXED_PRECISION);
			new_sample += fixed_mul_at(filter->state.input_coeff[2], filter->state.history[1], FILTER_FIXED_PRECISION);
			new_sample -= fixed_mul_at(filter->state.output_coeff[0], filter->state.output[0], FILTER_FIXED_PRECISION);
			new_sample -= fixed_mul_at(filter->state.output_coeff[1], filter->state.output[1], FILTER_FIXED_PRECISION);

			filter->state.history[1] = filter->state.history[0];
			filter->state.history[0] = sample << FILTER_FIXED_PRECISION;
			filter->state.output[1] = filter->state.output[0];
			filter->state.output[0] = new_sample;

			sample_t output = (sample_t)fixed_round_to_int_at(new_sample, FILTER_FIXED_PRECISION);
			*sample_data++ = output;
			*sample_data++ = output;
		}
	}
	else
	{
		int last_sample_index = (sample_count - 1) * 2;
		fixed_t sample = fixed_from_int_at(sample_data[last_sample_index], FILTER_FIXED_PRECISION);

		filter->state.history[1] = filter->state.history[0];
		filter->state.history[0] = sample;
		filter->state.output[1] = filter->state.output[0];
		filter->state.output[0] = sample;
	}
}
