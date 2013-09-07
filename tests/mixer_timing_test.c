#include "mixer_timing_test.h"
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "../system_constants.h"
#include "../mixer.h"
#include "../master_time.h"

static const int TEST_BUFFER_SIZE = 128;

void mixer_timing_test(int count)
{
	printf("*********************************************************************\n");
	printf("Mixer Timing Test\n");

	size_t src_buffer_size = count * TEST_BUFFER_SIZE;
	size_t dst_buffer_size = src_buffer_size * 2;
	sample_t* src_sample_buffer = (sample_t*)malloc(src_buffer_size * sizeof(sample_t));
	sample_t* dest_sample_buffer = (sample_t*)malloc(dst_buffer_size * sizeof(sample_t));

	int32_t mix_time = 0, start_mix_time, end_mix_time;

	memset(src_sample_buffer, 0, src_buffer_size * sizeof(sample_t));
	memset(dest_sample_buffer, 0, dst_buffer_size * sizeof(sample_t));
	start_mix_time = get_elapsed_cpu_time_ns();
	mixdown_mono_to_stereo(src_sample_buffer, PAN_MAX, PAN_MAX, src_buffer_size, dest_sample_buffer);
	end_mix_time = get_elapsed_cpu_time_ns();
	mix_time += end_mix_time - start_mix_time;

	printf("For %d iterations: C mix no clamp = %dns\n", count, mix_time);

	mix_time = 0;

	memset(src_sample_buffer, 127, src_buffer_size * sizeof(sample_t));
	memset(dest_sample_buffer, 127, src_buffer_size * sizeof(sample_t) * 2);
	start_mix_time = get_elapsed_cpu_time_ns();
	mixdown_mono_to_stereo(src_sample_buffer, PAN_MAX, PAN_MAX, src_buffer_size, dest_sample_buffer);
	end_mix_time = get_elapsed_cpu_time_ns();
	mix_time += end_mix_time - start_mix_time;

	printf("For %d iterations: C mix with clamp = %dns\n", count, mix_time);

	mix_time = 0;

	memset(src_sample_buffer, 127, src_buffer_size * sizeof(sample_t));
	memset(dest_sample_buffer, 127, src_buffer_size * sizeof(sample_t) * 2);
	start_mix_time = get_elapsed_cpu_time_ns();
	mixdown_mono_to_stereo_asm(src_sample_buffer, PAN_MAX, PAN_MAX, src_buffer_size, dest_sample_buffer);
	end_mix_time = get_elapsed_cpu_time_ns();
	mix_time += end_mix_time - start_mix_time;

	printf("For %d iterations: Asm mix with clamp = %dns\n", count, mix_time);

	mix_time = 0;

	memset(src_sample_buffer, 127, src_buffer_size * sizeof(sample_t));
	start_mix_time = get_elapsed_cpu_time_ns();
	copy_mono_to_stereo(src_sample_buffer, PAN_MAX, PAN_MAX, src_buffer_size, dest_sample_buffer);
	end_mix_time = get_elapsed_cpu_time_ns();
	mix_time += end_mix_time - start_mix_time;

	printf("For %d iterations: C copy stereo = %dns\n", count, mix_time);

	mix_time = 0;

	memset(src_sample_buffer, 127, src_buffer_size * sizeof(sample_t));
	start_mix_time = get_elapsed_cpu_time_ns();
	copy_mono_to_stereo_asm(src_sample_buffer, PAN_MAX, PAN_MAX, src_buffer_size, dest_sample_buffer);
	end_mix_time = get_elapsed_cpu_time_ns();
	mix_time += end_mix_time - start_mix_time;

	printf("For %d iterations: Asm copy stereo = %dns\n", count, mix_time);
}
