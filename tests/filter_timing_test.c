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
 * filter_timing_test.c
 *
 *  Created on: 15 Feb 2013
 *      Author: ntuckett
 */

#include "filter_timing_test.h"
#include <stdio.h>
#include "../system_constants.h"
#include "../fixed_point_math.h"
#include "../filter.h"
#include "../float_filter.h"
#include "../master_time.h"

extern void filter_apply_c(sample_t *sample_data, int sample_count, filter_state_t *filter_state);
extern void filter_apply_interp_c(sample_t *sample_data, int sample_count, filter_state_t *filter_state_current, filter_state_t *filter_state_last);
extern void filter_apply_asm(sample_t *sample_data, int sample_count, filter_state_t *filter_state);
extern void filter_apply_hp_asm(sample_t *sample_data, int sample_count, filter_state_t *filter_state);
extern void filter_apply_interp_asm(sample_t *sample_data, int sample_count, filter_state_t *filter_state_current, filter_state_t *filter_state_last);
extern void filter_apply_interp_hp_asm(sample_t *sample_data, int sample_count, filter_state_t *filter_state_current, filter_state_t *filter_state_last);

//
// These C implementations are for timing and reference purposes; they may not produce correct sounding results
// due to changes in the precision used for calculating coefficients. However the fundamental algorithm and math
// operations remain correct, so are ok for comparing timings.
//
#define FILTER_INTERNAL_PRECISION	14

__attribute__((always_inline)) inline sample_t filter_sample(sample_t sample, filter_state_t *filter_state)
{
	fixed_t new_sample;
	new_sample =  (fixed_t)sample * filter_state->input_coeff[0];
	new_sample += filter_state->input_coeff[2] * filter_state->history[1];
	new_sample += filter_state->input_coeff[1] * filter_state->history[0];
	new_sample += filter_state->output_coeff[1] * filter_state->output[1];
	new_sample += filter_state->output_coeff[0] * filter_state->output[0];
	new_sample  = fixed_round_to_int_at(new_sample, FILTER_INTERNAL_PRECISION);

	filter_state->history[1] = filter_state->history[0];
	filter_state->history[0] = sample;
	filter_state->output[1] = filter_state->output[0];
	filter_state->output[0] = new_sample;

	return (sample_t)new_sample;
}

void filter_apply_c(sample_t *sample_data, int sample_count, filter_state_t *filter_state)
{
	for (int i = 0; i < sample_count; i++)
	{
		sample_t output = filter_sample(*sample_data, filter_state);
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

		interpolator_new += interpolation_delta;
		interpolator_old -= interpolation_delta;
	}
}

static const int TEST_BUFFER_SIZE = 128;

void filter_timing_test(int count)
{
	filter_t fixed_math_filter;
	filter_t fixed_math_filter2;
	sample_t int_sample_buffer[TEST_BUFFER_SIZE];
	float_filter_t float_math_filter;
	float float_sample_buffer[TEST_BUFFER_SIZE];

	printf("*********************************************************************\n");
	printf("Filter Timing Test\n");

	filter_init(&fixed_math_filter);
	fixed_math_filter.definition.type = FILTER_LPF;
	fixed_math_filter.definition.frequency = DOUBLE_TO_FIXED(880.0);
	fixed_math_filter.definition.q = FIXED_HALF;
	filter_update(&fixed_math_filter);
	fixed_math_filter2 = fixed_math_filter;

	float_filter_init(&float_math_filter);
	float_math_filter.definition.type = FILTER_LPF;
	float_math_filter.definition.frequency = 880.0f;
	float_math_filter.definition.q = 0.5f;
	float_filter_update(&float_math_filter);

	int32_t start_fixed_time = get_elapsed_time_ms();

	for (int i = 0; i < count; i++)
	{
		filter_apply(&fixed_math_filter, int_sample_buffer, TEST_BUFFER_SIZE);
	}

	int32_t start_float_time = get_elapsed_time_ms();

	for (int i = 0; i < count; i++)
	{
		float_filter_apply(&float_math_filter, float_sample_buffer, TEST_BUFFER_SIZE);
	}

	int32_t end_time = get_elapsed_time_ms();

	printf("For %d iterations: fixed = %dms, float = %dms\n", count, start_float_time - start_fixed_time, end_time - start_float_time);

	int32_t start_c_time = get_elapsed_time_ms();

	for (int i = 0; i < count; i++)
	{
		filter_apply_c(int_sample_buffer, TEST_BUFFER_SIZE, &fixed_math_filter.state);
	}

	int32_t start_asm_time = get_elapsed_time_ms();

	for (int i = 0; i < count; i++)
	{
		filter_apply_asm(int_sample_buffer, TEST_BUFFER_SIZE, &fixed_math_filter.state);
	}

	end_time = get_elapsed_time_ms();

	printf("For %d iterations (fixed): c = %dms, asm = %dms\n", count, start_asm_time - start_c_time, end_time - start_asm_time);

	start_c_time = get_elapsed_time_ms();

	for (int i = 0; i < count; i++)
	{
		filter_apply_interp_c(int_sample_buffer, TEST_BUFFER_SIZE, &fixed_math_filter.state, &fixed_math_filter2.state);
	}

	start_asm_time = get_elapsed_time_ms();

	for (int i = 0; i < count; i++)
	{
		filter_apply_interp_asm(int_sample_buffer, TEST_BUFFER_SIZE, &fixed_math_filter.state, &fixed_math_filter2.state);
	}

	end_time = get_elapsed_time_ms();

	printf("For %d iterations (fixed interp): c = %dms, asm = %dms\n", count, start_asm_time - start_c_time, end_time - start_asm_time);

	start_asm_time = get_elapsed_time_ms();

	for (int i = 0; i < count; i++)
	{
		filter_apply_hp_asm(int_sample_buffer, TEST_BUFFER_SIZE, &fixed_math_filter.state);
	}

	end_time = get_elapsed_time_ms();

	printf("For %d iterations (fixed): hp asm = %dms\n", count, end_time - start_asm_time);

	start_asm_time = get_elapsed_time_ms();

	for (int i = 0; i < count; i++)
	{
		filter_apply_interp_hp_asm(int_sample_buffer, TEST_BUFFER_SIZE, &fixed_math_filter.state, &fixed_math_filter2.state);
	}

	end_time = get_elapsed_time_ms();

	printf("For %d iterations (fixed interp): hp asm = %dms\n", count, end_time - start_asm_time);
}
