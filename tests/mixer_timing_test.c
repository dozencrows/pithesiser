#include "mixer_timing_test.h"
#include <stdio.h>
#include <memory.h>
#include "../system_constants.h"
#include "../mixer.h"
#include "../master_time.h"

static const int TEST_BUFFER_SIZE = 128;

void mixer_timing_test(int count)
{
	printf("*********************************************************************\n");
	printf("Mixer Timing Test\n");

	sample_t src_sample_buffer[TEST_BUFFER_SIZE];
	sample_t dest_sample_buffer[TEST_BUFFER_SIZE * 2];

	int32_t mix_time = 0;

	for (int i = 0; i < count; i++)
	{
		memset(src_sample_buffer, 0, TEST_BUFFER_SIZE * sizeof(sample_t));
		memset(dest_sample_buffer, 0, TEST_BUFFER_SIZE * sizeof(sample_t) * 2);

		int32_t start_mix_time = get_elapsed_time_ms();
		mixdown_mono_to_stereo(src_sample_buffer, PAN_MAX, PAN_MAX, TEST_BUFFER_SIZE, dest_sample_buffer);
		int32_t end_mix_time = get_elapsed_time_ms();
		mix_time += end_mix_time - start_mix_time;
	}

	printf("For %d iterations: C no clamp = %dms\n", count, mix_time);

	mix_time = 0;

	for (int i = 0; i < count; i++)
	{
		memset(src_sample_buffer, 127, TEST_BUFFER_SIZE * sizeof(sample_t));
		memset(dest_sample_buffer, 127, TEST_BUFFER_SIZE * sizeof(sample_t) * 2);

		int32_t start_mix_time = get_elapsed_time_ms();
		mixdown_mono_to_stereo(src_sample_buffer, PAN_MAX, PAN_MAX, TEST_BUFFER_SIZE, dest_sample_buffer);
		int32_t end_mix_time = get_elapsed_time_ms();
		mix_time += end_mix_time - start_mix_time;
	}

	printf("For %d iterations: C with clamp = %dms\n", count, mix_time);

	mix_time = 0;

	for (int i = 0; i < count; i++)
	{
		memset(src_sample_buffer, 127, TEST_BUFFER_SIZE * sizeof(sample_t));
		memset(dest_sample_buffer, 127, TEST_BUFFER_SIZE * sizeof(sample_t) * 2);

		int32_t start_mix_time = get_elapsed_time_ms();
		copy_mono_to_stereo(src_sample_buffer, PAN_MAX, PAN_MAX, TEST_BUFFER_SIZE, dest_sample_buffer);
		int32_t end_mix_time = get_elapsed_time_ms();
		mix_time += end_mix_time - start_mix_time;
	}

	printf("For %d iterations: C copy stereo = %dms\n", count, mix_time);

	mix_time = 0;

	for (int i = 0; i < count; i++)
	{
		memset(src_sample_buffer, 127, TEST_BUFFER_SIZE * sizeof(sample_t));
		memset(dest_sample_buffer, 127, TEST_BUFFER_SIZE * sizeof(sample_t) * 2);

		int32_t start_mix_time = get_elapsed_time_ms();
		copy_mono_to_stereo_asm(src_sample_buffer, PAN_MAX, PAN_MAX, TEST_BUFFER_SIZE, dest_sample_buffer);
		int32_t end_mix_time = get_elapsed_time_ms();
		mix_time += end_mix_time - start_mix_time;
	}

	printf("For %d iterations: Asm copy stereo = %dms\n", count, mix_time);
}
