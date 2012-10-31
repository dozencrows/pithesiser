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
#include "oscillator.h"

#define DURATION_SECONDS	5
#define MIN_FREQUENCY		440.0f
#define MAX_FREQUENCY		880.0f

int main()
{
	if (alsa_initialise("hw:1", 128) < 0)
	{
		exit(EXIT_FAILURE);
	}

	if (midi_initialise("/dev/midi2") < 0)
	{
		exit(EXIT_FAILURE);
	}

	int elapsed_samples = 0;

	oscillator_t oscillator;

	osc_init(&oscillator);

	while ((elapsed_samples = alsa_get_samples_output()) < SAMPLE_RATE * DURATION_SECONDS)
	{
		if (midi_get_controller_value(0, 14) < 63)
		{
			oscillator.waveform = WAVE_SINE;
		}
		else
		{
			oscillator.waveform = WAVE_SAW;
		}

		alsa_sync_with_audio_output();
		int write_buffer_index = alsa_lock_next_write_buffer();
		void* buffer_data;
		int buffer_samples;
		alsa_get_buffer_params(write_buffer_index, &buffer_data, &buffer_samples);

		float freq_control = (float) midi_get_controller_value(0, 2) / MIDI_MAX_CONTROLLER_VALUE;
		oscillator.frequency = MIN_FREQUENCY + (MAX_FREQUENCY - MIN_FREQUENCY) * freq_control;
		osc_output(&oscillator, buffer_samples, buffer_data);
		alsa_unlock_buffer(write_buffer_index);
	}

	alsa_deinitialise();
	midi_deinitialise();

	printf("Done: %d samples output, %d xruns\n", elapsed_samples, alsa_get_xruns_count());
	return 0;
}
