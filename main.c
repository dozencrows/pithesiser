/*
 * main.c
 *
 *  Created on: 29 Oct 2012
 *      Author: ntuckett
 */

#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include "alsa.h"
#include "midi.h"

#define DURATION_SECONDS	10
#define MIN_FREQUENCY		440.0f
#define MAX_FREQUENCY		880.0f

void generate_wave(void* data, int sample_count, float* wave_phase, float frequency)
{
	static float max_phase = M_PI * 2.0f;
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
	if (alsa_initialise("hw:1") < 0)
	{
		exit(EXIT_FAILURE);
	}

	if (midi_initialise("/dev/midi2") < 0)
	{
		exit(EXIT_FAILURE);
	}

	int elapsed_samples = 0;
	float wave_phase = 0.0f;
	while ((elapsed_samples = alsa_get_samples_output()) < SAMPLE_RATE * DURATION_SECONDS)
	//while (1)
	{
		alsa_sync_with_audio_output();
		int write_buffer_index = alsa_lock_next_write_buffer();
		void* buffer_data;
		int buffer_samples;
		alsa_get_buffer_params(write_buffer_index, &buffer_data, &buffer_samples);

		float freq_control = (float) midi_get_controller_value(0, 2) / MIDI_MAX_CONTROLLER_VALUE;
		float frequency = MIN_FREQUENCY + (MAX_FREQUENCY - MIN_FREQUENCY) * freq_control;
		generate_wave(buffer_data, buffer_samples, &wave_phase, frequency);
		alsa_unlock_buffer(write_buffer_index);
	}

	alsa_deinitialise();
	midi_deinitialise();

	printf("Done: %d samples, %d xruns\n", elapsed_samples, alsa_get_xruns_count());
	return 0;
}
