// Pithesiser - a software synthesiser for Raspberry Pi
// Copyright (C) 2015 Nicholas Tuckett
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

/*
 * filter.c
 *
 *  Created on: 3 Feb 2013
 *      Author: ntuckett
 */

#include "filter.h"
#include <memory.h>
#include "fixed_point_math.h"

extern void filter_apply_hp_asm(sample_t *sample_data, int sample_count, filter_state_t *filter_state);
extern void filter_apply_interp_hp_asm(sample_t *sample_data, int sample_count, filter_state_t *filter_state_current, filter_state_t *filter_state_last);

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
	filter->definition.type = FILTER_PASS;
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

	fixed_wide_t q = filter->definition.q;
	fixed_wide_t alpha	= fixed_divide_wide(sin_w0, q * 2);

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
			a[1] = -2 * cos_w0;
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
			a[1] = -2 * cos_w0;
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

	filter->state.input_coeff[0] = fixed_divide_wide(b[0], a[0]);
	filter->state.input_coeff[1] = fixed_divide_wide(b[1], a[0]);
	filter->state.input_coeff[2] = fixed_divide_wide(b[2], a[0]);

	filter->state.output_coeff[0] = -fixed_divide_wide(a[1], a[0]);
	filter->state.output_coeff[1] = -fixed_divide_wide(a[2], a[0]);

//	printf("%9.03f -> I: %08x %08x %08x  O: %08x %08x\n", (float)filter->definition.frequency / (float)FILTER_FIXED_ONE,
//			filter->state.input_coeff[0], filter->state.input_coeff[1], filter->state.input_coeff[2],
//			filter->state.output_coeff[0], filter->state.output_coeff[1]);
}

void filter_silence(filter_t *filter)
{
	clear_history(&filter->state);
	filter->last_type = FILTER_PASS;	// Avoid interpolation on next non-silence when params updated.
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

int filter_definitions_same(filter_definition_t *definition1, filter_definition_t *definition2)
{
	return definition1 == definition2 || (definition1->type == definition2->type
											&& definition1->frequency == definition2->frequency
											&& definition1->q == definition2->q
											&& definition1->gain == definition2->gain);
}
