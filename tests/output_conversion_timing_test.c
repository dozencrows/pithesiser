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
