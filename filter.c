/*
 * filter.c
 *
 *  Created on: 3 Feb 2013
 *      Author: ntuckett
 */

#include "filter.h"
#include <memory.h>
#include <math.h>

//#define FLOAT_FILTER

#if !defined(FLOAT_FILTER)
#include "fixed_point_math.h"
#endif

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
#if defined(FLOAT_FILTER)
	double frequency	= (double)filter->definition.frequency/(double)FIXED_ONE;
	double q = (double)filter->definition.q/(double)FIXED_ONE;

	double w0 = 2.0f * M_PI * frequency / (double)SYSTEM_SAMPLE_RATE;
	double cos_w0	= cos(w0);
	double sin_w0	= sin(w0);
	double alpha	= sin_w0 / (q + q);

	double a[3];
	double b[3];

	switch(filter->definition.type)
	{
		case FILTER_LPF:
		{
			double tmp = 1.0f - cos_w0;
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
			double tmp = 1.0f + cos_w0;
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

	filter->state.input_coeff[0] = (fixed_wide_t)((b[0] / a[0]) * (double)FIXED_ONE);
	filter->state.input_coeff[1] = (fixed_wide_t)((b[1] / a[0]) * (double)FIXED_ONE);
	filter->state.input_coeff[2] = (fixed_wide_t)((b[2] / a[0]) * (double)FIXED_ONE);

	filter->state.output_coeff[0] = (fixed_wide_t)((a[1] / a[0]) * (double)FIXED_ONE);
	filter->state.output_coeff[1] = (fixed_wide_t)((a[2] / a[0]) * (double)FIXED_ONE);
#else
	fixed_wide_t w0 = (((fixed_wide_t)(FIXED_PI) * (fixed_wide_t)filter->definition.frequency)) >> FIXED_PRECISION;
	w0 = (2 * w0) / (fixed_wide_t)SYSTEM_SAMPLE_RATE;
	fixed_t cos_w0, sin_w0;

	fixed_sin_cos((fixed_t)w0, &sin_w0, &cos_w0);

	fixed_t alpha	= ((fixed_wide_t)sin_w0 << FIXED_PRECISION) / ((fixed_wide_t)(filter->definition.q) * 2);

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


	filter->state.input_coeff[0] = (b[0] << FIXED_PRECISION) / a[0];
	filter->state.input_coeff[1] = (b[1] << FIXED_PRECISION) / a[0];
	filter->state.input_coeff[2] = (b[2] << FIXED_PRECISION) / a[0];

	filter->state.output_coeff[0] = (a[1] << FIXED_PRECISION)/ a[0];
	filter->state.output_coeff[1] = (a[2] << FIXED_PRECISION)/ a[0];
#endif
	clear_history(filter);
}

void filter_apply(filter_t *filter, sample_t *sample_data, int sample_count)
{
	if (filter->definition.type != FILTER_PASS)
	{
		for (int i = 0; i < sample_count; i++)
		{
			fixed_wide_t sample = (fixed_wide_t)(*sample_data);
			fixed_wide_t new_sample;
			new_sample =  sample * filter->state.input_coeff[0];
			new_sample += (filter->state.input_coeff[1] * filter->state.history[0]) >> FIXED_PRECISION;
			new_sample += (filter->state.input_coeff[2] * filter->state.history[1]) >> FIXED_PRECISION;
			new_sample -= (filter->state.output_coeff[0] * filter->state.output[0]) >> FIXED_PRECISION;
			new_sample -= (filter->state.output_coeff[1] * filter->state.output[1]) >> FIXED_PRECISION;

			filter->state.history[1] = filter->state.history[0];
			filter->state.history[0] = sample << FIXED_PRECISION;
			filter->state.output[1] = filter->state.output[0];
			filter->state.output[0] = new_sample;

			sample_t output = (sample_t)((new_sample + FIXED_HALF) >> FIXED_PRECISION);
			*sample_data++ = output;
			*sample_data++ = output;
		}
	}
	else
	{
		int last_sample_index = (sample_count - 1) * 2;
		fixed_wide_t sample = (fixed_wide_t)(sample_data[last_sample_index]) << FIXED_PRECISION;

		filter->state.history[1] = filter->state.history[0];
		filter->state.history[0] = sample;
		filter->state.output[1] = filter->state.output[0];
		filter->state.output[0] = sample;
	}
}
