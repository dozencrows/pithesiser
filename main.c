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
#define NOTE_LEVEL				8192

typedef struct
{
	int	midi_channel;
	int pitch_controller;
	int wave_controller;
	int trigger_controller;
	int level_controller;
	int last_note;
	int current_note;
	int play_counter;
} voice_t;

#define VOICE_COUNT	8
voice_t voice[VOICE_COUNT] =
{
	{ MIDI_CONTROL_CHANNEL, 0x0e, 0x21, 0x17, 0x02, -1, -1, 0 },
	{ MIDI_CONTROL_CHANNEL, 0x0f, 0x22, 0x18, 0x03, -1, -1, 0 },
	{ MIDI_CONTROL_CHANNEL, 0x10, 0x23, 0x19, 0x04, -1, -1, 0 },
	{ MIDI_CONTROL_CHANNEL, 0x11, 0x24, 0x1a, 0x05, -1, -1, 0 },
	{ MIDI_CONTROL_CHANNEL, 0x0e, 0x21, 0x17, 0x02, -1, -1, 0 },
	{ MIDI_CONTROL_CHANNEL, 0x0f, 0x22, 0x18, 0x03, -1, -1, 0 },
	{ MIDI_CONTROL_CHANNEL, 0x10, 0x23, 0x19, 0x04, -1, -1, 0 },
	{ MIDI_CONTROL_CHANNEL, 0x11, 0x24, 0x1a, 0x05, -1, -1, 0 },
};

oscillator_t oscillator[VOICE_COUNT];

float master_level = 1.0f;
waveform_type_t master_waveform = WAVE_FIRST;

void process_midi_events()
{
	int midi_events = midi_get_event_count();

	while (midi_events-- > 0)
	{
		midi_event_t midi_event;
		midi_pop_event(&midi_event);
		if (midi_event.type == 0x90)
		{
			int candidate_voice = -1;
			int lowest_play_counter = INT_MAX;

			for (int i = 0; i < VOICE_COUNT; i++)
			{
				if (voice[i].current_note == -1)
				{
					candidate_voice = i;
					break;
				}
				else
				{
					if (voice[i].play_counter < lowest_play_counter)
					{
						lowest_play_counter = voice[i].play_counter;
						candidate_voice = i;
					}
				}
			}

			voice[candidate_voice].current_note = midi_event.data[0];
		}
		else if (midi_event.type == 0x80)
		{
			for (int i = 0; i < VOICE_COUNT; i++)
			{
				if (voice[i].last_note == midi_event.data[0])
				{
					voice[i].current_note = -1;
				}
			}
		}
	}

	master_level = (float) midi_get_controller_value(voice[0].midi_channel, voice[0].level_controller) / MIDI_MAX_CONTROLLER_VALUE;

	if (midi_get_controller_changed(voice[0].midi_channel, voice[0].wave_controller))
	{
		if (midi_get_controller_value(voice[0].midi_channel, voice[0].wave_controller) > 63)
		{
			master_waveform++;
			if (master_waveform > WAVE_LAST)
			{
				master_waveform = WAVE_FIRST;
			}
		}
	}
}

void process_audio()
{
	int write_buffer_index = alsa_lock_next_write_buffer();
	void* buffer_data;
	int buffer_samples;
	alsa_get_buffer_params(write_buffer_index, &buffer_data, &buffer_samples);

	int first_audible_voice = -1;

	for (int i = 0; i < VOICE_COUNT; i++)
	{
		if (voice[i].current_note != voice[i].last_note)
		{
			if (voice[i].current_note == -1)
			{
				oscillator[i].level = 0;
			}
			else
			{
				oscillator[i].frequency = midi_get_note_frequency(voice[i].current_note);
				oscillator[i].waveform = master_waveform;
				oscillator[i].phase_accumulator = 0;
			}

			voice[i].last_note = voice[i].current_note;
		}

		if (voice[i].current_note != -1)
		{
			oscillator[i].level = NOTE_LEVEL * master_level;
			if (oscillator[i].level > 0)
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
	}

	if (first_audible_voice < 0)
	{
		memset(buffer_data, 0, buffer_samples * sizeof(int16_t) * 2);
	}

	alsa_unlock_buffer(write_buffer_index);
}

int main(int argc, char **argv)
{
	if (alsa_initialise("hw:1", 128) < 0)
	{
		exit(EXIT_FAILURE);
	}

	if (midi_initialise("/dev/midi2", "/dev/midi3") < 0)
	{
		exit(EXIT_FAILURE);
	}

	waveform_initialise();

	for (int i = 0; i < VOICE_COUNT; i++)
	{
		osc_init(&oscillator[i]);
	}

	int profiling = 0;

	while (1)
	{
		if (midi_get_controller_value(MIDI_CONTROL_CHANNEL, EXIT_CONTROLLER) > 63)
		{
			break;
		}

		process_midi_events();

		if (!profiling && midi_get_controller_changed(MIDI_CONTROL_CHANNEL, PROFILE_CONTROLLER))
		{
			if (argc > 1 && midi_get_controller_value(MIDI_CONTROL_CHANNEL, PROFILE_CONTROLLER) > 63)
			{
				ProfilerStart(argv[1]);
				profiling = 1;
			}
		}

		alsa_sync_with_audio_output();

		process_audio();
	}

	if (argc > 1)
	{
		ProfilerStop();
	}

	alsa_deinitialise();
	midi_deinitialise();

	printf("Done: %d xruns\n", alsa_get_xruns_count());
	return 0;
}
