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

static const int TEST_BUFFER_SIZE = 128;

void filter_timing_test(int count)
{
	filter_t fixed_math_filter;
	sample_t int_sample_buffer[TEST_BUFFER_SIZE * 2];
	float_filter_t float_math_filter;
	float float_sample_buffer[TEST_BUFFER_SIZE * 2];

	printf("*********************************************************************\n");
	printf("Filter Timing Test\n");

	filter_init(&fixed_math_filter);
	fixed_math_filter.definition.type = FILTER_LPF;
	fixed_math_filter.definition.frequency = DOUBLE_TO_FIXED(880.0);
	fixed_math_filter.definition.q = FIXED_HALF;
	filter_update(&fixed_math_filter);

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
}
