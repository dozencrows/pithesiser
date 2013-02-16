/*
 * output_conversion_timing_test.c
 *
 *  Created on: 16 Feb 2013
 *      Author: ntuckett
 */
#include <stdio.h>
#include <sys/types.h>
#include <limits.h>
#include "../master_time.h"

static const int TEST_BUFFER_SIZE = 128;

static void float_to_int_16_cast(float *float_sample_buffer, int16_t *int_sample_buffer, int sample_count)
{
	for (int i = 0; i < sample_count * 2; i++)
	{
		*int_sample_buffer++ = *float_sample_buffer++ * SHRT_MAX;
	}
}

static void float_to_int16_timing_test(int count)
{
	float	float_sample_buffer[TEST_BUFFER_SIZE * 2];
	int16_t	int_sample_buffer[TEST_BUFFER_SIZE * 2];

	int32_t start_time = get_elapsed_time_ms();
	for (int i = 0; i < count; i++)
	{
		float_to_int_16_cast(float_sample_buffer, int_sample_buffer, TEST_BUFFER_SIZE);
	}

	int32_t end_time = get_elapsed_time_ms();

	printf("Cast & scale float to int16: for %d iterations %dms\n", count, end_time - start_time);
}

void output_conversion_timing_test(int count)
{
	printf("*********************************************************************\n");
	printf("Output conversion Timing Test\n");

	float_to_int16_timing_test(count);
}
