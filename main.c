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
#include <memory.h>
#include <gperftools/profiler.h>
#include "alsa.h"
#include "midi.h"
#include "waveform.h"
#include "oscillator.h"

#define MIN_FREQUENCY			55.0f
#define MAX_FREQUENCY			1760.0f
#define MIDI_CONTROL_CHANNEL	0
#define EXIT_CONTROLLER			0x2e
#define PROFILE_CONTROLLER		0x2c

typedef struct
{
	int	midi_channel;
	int pitch_controller;
	int wave_controller;
	int trigger_controller;
	int level_controller;
} voice_t;

#define VOICE_COUNT	4
voice_t voice[VOICE_COUNT] =
{
	{ MIDI_CONTROL_CHANNEL, 0x0e, 0x21, 0x17, 0x02 },
	{ MIDI_CONTROL_CHANNEL, 0x0f, 0x22, 0x18, 0x03 },
	{ MIDI_CONTROL_CHANNEL, 0x10, 0x23, 0x19, 0x04 },
	{ MIDI_CONTROL_CHANNEL, 0x11, 0x24, 0x1a, 0x05 },
};

int main(int argc, char **argv)
{
	if (alsa_initialise("hw:2", 128) < 0)
	{
		exit(EXIT_FAILURE);
	}

	if (midi_initialise("/dev/midi1") < 0)
	{
		exit(EXIT_FAILURE);
	}

	waveform_initialise();

	int elapsed_samples = 0;

	oscillator_t oscillator[VOICE_COUNT];

	for (int i = 0; i < VOICE_COUNT; i++)
	{
		osc_init(&oscillator[i]);
	}

	int profiling = 0;
	int current_note = -1;

	while (1)
	{
		int midi_events = midi_get_event_count();

		while (midi_events-- > 0)
		{
			midi_event_t midi_event;
			midi_pop_event(&midi_event);
			if (midi_event.type == 0x90)
			{
				current_note = midi_event.data[0];
			}
			else if (midi_event.type == 0x80)
			{
				current_note = -1;
			}
		}

		if (midi_get_controller_value(MIDI_CONTROL_CHANNEL, EXIT_CONTROLLER) > 63)
		{
			break;
		}

		if (!profiling && midi_get_controller_changed(MIDI_CONTROL_CHANNEL, PROFILE_CONTROLLER))
		{
			if (argc > 1 && midi_get_controller_value(MIDI_CONTROL_CHANNEL, PROFILE_CONTROLLER) > 63)
			{
				ProfilerStart(argv[1]);
				profiling = 1;
			}
		}

		alsa_sync_with_audio_output();
		int write_buffer_index = alsa_lock_next_write_buffer();
		void* buffer_data;
		int buffer_samples;
		alsa_get_buffer_params(write_buffer_index, &buffer_data, &buffer_samples);

		int first_audible_voice = -1;

		for (int i = 0; i < VOICE_COUNT; i++)
		{
			float level_control = (float) midi_get_controller_value(voice[i].midi_channel, voice[i].level_controller) / MIDI_MAX_CONTROLLER_VALUE;
			oscillator[i].level = SHRT_MAX * level_control;

			if (midi_get_controller_changed(voice[i].midi_channel, voice[i].wave_controller))
			{
				if (midi_get_controller_value(voice[i].midi_channel, voice[i].wave_controller) > 63)
				{
					oscillator[i].waveform++;
					oscillator[i].phase_accumulator = 0;
					if (oscillator[i].waveform > WAVE_LAST)
					{
						oscillator[i].waveform = WAVE_FIRST;
					}
				}
			}

			int note_trigger = 0;

			if (i == 0)
			{
				if (current_note != -1)
				{
					oscillator[i].frequency = midi_get_note_frequency(current_note);
					oscillator[i].level = 16384;
					oscillator[i].waveform = PROCEDURAL_SINE;
					note_trigger = 1;
				}
				else
				{
					oscillator[i].level = 0;
				}
			}
			else
			{
				float freq_control = (float) midi_get_controller_value(voice[i].midi_channel, voice[i].pitch_controller) / MIDI_MAX_CONTROLLER_VALUE;
				oscillator[i].frequency = MIN_FREQUENCY + (MAX_FREQUENCY - MIN_FREQUENCY) * freq_control;
				note_trigger = midi_get_controller_value(voice[i].midi_channel, voice[i].trigger_controller) > 63;
			}

			if (note_trigger && oscillator[i].level > 0)
			{
				if (first_audible_voice < 0)
				{
					first_audible_voice = i;
					osc_output(&oscillator[i], buffer_samples, buffer_data);
				}
				else
				{
					osc_mix_output(&oscillator[i], buffer_samples, buffer_data);
				}
			}
		}

		if (first_audible_voice < 0)
		{
			memset(buffer_data, 0, buffer_samples * sizeof(int16_t) * 2);
		}

		alsa_unlock_buffer(write_buffer_index);
	}

	if (argc > 1)
	{
		ProfilerStop();
	}

	alsa_deinitialise();
	midi_deinitialise();

	printf("Done: %d samples output, %d xruns\n", elapsed_samples, alsa_get_xruns_count());
	return 0;
}
