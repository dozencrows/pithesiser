/*
 * main.c
 *
 *  Created on: 29 Oct 2012
 *      Author: ntuckett
 */

#include <unistd.h>
#include <math.h>
#include <limits.h>
#include "alsa.h"

#define DURATION_SECONDS	10

void generate_wave(void* data, int sample_count, float* wave_phase)
{
	static float max_phase = 2.0f * (float)M_PI;
	static float frequency = 440.0f;
	float phase_step = max_phase * frequency / SAMPLE_RATE;

	float phase = *wave_phase;
	short* sample_ptr = (short*) data;

	while (sample_count > 0)
	{
		short value = sinf(phase) * SHRT_MAX;
		*sample_ptr++ = value;
		*sample_ptr++ = value;
		phase += phase_step;
		if (phase > max_phase)
		{
			phase -= max_phase;
		}
		sample_count--;
	}

	*wave_phase = phase;
}

int main()
{
	//alsa_initialise("hw:0");
	alsa_initialise("hw:1");
	int elapsed_samples = 0;

	float wave_phase = 0.0f;
	while ((elapsed_samples = alsa_get_samples_output()) < SAMPLE_RATE * DURATION_SECONDS)
	{
		alsa_sync_with_audio_output();
		int write_buffer_index = alsa_lock_next_write_buffer();
		void* buffer_data;
		int buffer_samples;
		alsa_get_buffer_params(write_buffer_index, &buffer_data, &buffer_samples);
		generate_wave(buffer_data, buffer_samples, &wave_phase);
		alsa_unlock_buffer(write_buffer_index);
	}
	alsa_deinitialise();

	printf("Done: %d samples, %d xruns\n", elapsed_samples, alsa_get_xruns_count());
	return 0;
}
