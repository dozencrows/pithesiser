/*
 * filter.c
 *
 *  Created on: 3 Feb 2013
 *      Author: ntuckett
 */

#include "filter.h"
#include <memory.h>
#include "fixed_point_math.h"

extern void filter_apply_asm(sample_t *sample_data, int sample_count, filter_state_t *filter_state);
extern void filter_apply_hp_asm(sample_t *sample_data, int sample_count, filter_state_t *filter_state);
extern void filter_apply_interp_asm(sample_t *sample_data, int sample_count, filter_state_t *filter_state_current, filter_state_t *filter_state_last);

#define FILTER_PRECISION_DELTA  (FIXED_PRECISION - FILTER_FIXED_PRECISION)

static void clear_history(filter_state_t *state)
{
	memset(state->history, 0, sizeof(state->history));
	memset(state->output, 0, sizeof(state->output));
}

static void clear_state(filter_state_t *state)
{
	memset(state->input_coeff, 0, sizeof(state->input_coeff));
	memset(state->output_coeff, 0, sizeof(state->output_coeff));
	clear_history(state);
}

void filter_init(filter_t *filter)
{
	clear_state(&filter->state);
	clear_state(&filter->last_state);
	filter->updated = 0;
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

	if (filter->definition.type == filter->last_type)
	{
		filter->last_state = filter->state;
		filter->updated = 1;
	}
	else
	{
		filter->last_type = filter->definition.type;
		filter->updated = 0;
	}
	clear_state(&filter->state);

	filter->state.input_coeff[0] = fixed_divide_at(b[0], a[0], FILTER_FIXED_PRECISION);
	filter->state.input_coeff[1] = fixed_divide_at(b[1], a[0], FILTER_FIXED_PRECISION);
	filter->state.input_coeff[2] = fixed_divide_at(b[2], a[0], FILTER_FIXED_PRECISION);

	filter->state.output_coeff[0] = -fixed_divide_at(a[1], a[0], FILTER_FIXED_PRECISION);
	filter->state.output_coeff[1] = -fixed_divide_at(a[2], a[0], FILTER_FIXED_PRECISION);

	//	printf("%d,%d,%Ld,%d,%d,%d,", filter->definition.frequency, filter->definition.q, w0, cos_w0, sin_w0, alpha);
	//	printf("%d,%d,%d,%d,%d,%d,", a[0], a[1], a[2], b[0], b[1], b[2]);
	//	printf("%d,%d,%d,", filter->state.input_coeff[0], filter->state.input_coeff[1], filter->state.input_coeff[2]);
	//	printf("%d,%d\n", filter->state.output_coeff[0], filter->state.output_coeff[1]);
}

__attribute__((always_inline)) inline sample_t filter_sample(sample_t sample, filter_state_t *filter_state)
{
	fixed_t new_sample;
	new_sample =  (fixed_t)sample * filter_state->input_coeff[0];
	new_sample += filter_state->input_coeff[2] * filter_state->history[1];
	new_sample += filter_state->input_coeff[1] * filter_state->history[0];
	new_sample += filter_state->output_coeff[1] * filter_state->output[1];
	new_sample += filter_state->output_coeff[0] * filter_state->output[0];
	new_sample  = fixed_round_to_int_at(new_sample, FILTER_FIXED_PRECISION);

	filter_state->history[1] = filter_state->history[0];
	filter_state->history[0] = sample;
	filter_state->output[1] = filter_state->output[0];
	filter_state->output[0] = new_sample;

	return (sample_t)new_sample;
}

void filter_silence(filter_t *filter)
{
	clear_history(&filter->state);
}

void filter_apply_c(sample_t *sample_data, int sample_count, filter_state_t *filter_state)
{
	for (int i = 0; i < sample_count; i++)
	{
		sample_t output = filter_sample(*sample_data, filter_state);
		*sample_data++ = output;
		*sample_data++ = output;
	}
}

#define INTERP_PRECISION	15
#define INTERP_ONE			(1 << INTERP_PRECISION)

void filter_apply_interp_c(sample_t *sample_data, int sample_count, filter_state_t *filter_state_current, filter_state_t *filter_state_last)
{
	int32_t interpolation_delta = (1 << INTERP_PRECISION) / sample_count;
	int32_t interpolator_old = INTERP_ONE;
	int32_t interpolator_new = 0;

	for (int i = 0; i < sample_count; i++)
	{
		sample_t old_output = filter_sample(*sample_data, filter_state_last);
		sample_t new_output = filter_sample(*sample_data, filter_state_current);

		sample_t output = (((int32_t)new_output * interpolator_new) >> INTERP_PRECISION) + (((int32_t)old_output * interpolator_old) >> INTERP_PRECISION);
		*sample_data++ = output;
		*sample_data++ = output;

		interpolator_new += interpolation_delta;
		interpolator_old -= interpolation_delta;
	}
}

void filter_apply(filter_t *filter, sample_t *sample_data, int sample_count)
{
	if (filter->definition.type != FILTER_PASS)
	{
		if (filter->updated)
		{
			filter_apply_interp_hp_asm(sample_data, sample_count, &filter->state, &filter->last_state);
			filter->updated = 0;
		}
		else
		{
			filter_apply_hp_asm(sample_data, sample_count, &filter->state);
		}
	}
	else
	{
		int last_sample_index = (sample_count - 1) * 2;
		fixed_t sample = (fixed_t)sample_data[last_sample_index];

		filter->state.history[1] = filter->state.history[0];
		filter->state.history[0] = sample;
		filter->state.output[1] = filter->state.output[0];
		filter->state.output[0] = sample;
	}
}
