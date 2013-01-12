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
#include "envelope.h"
#include "gfx.h"
#include "gfx_event.h"
#include "gfx_event_types.h"
#include "gfx_wave_render.h"
#include "master_time.h"

#define MIN_FREQUENCY			55.0f
#define MAX_FREQUENCY			1760.0f
#define MIDI_CONTROL_CHANNEL	0
#define EXIT_CONTROLLER			0x2e
#define PROFILE_CONTROLLER		0x2c
#define OSCILLOSCOPE_CONTROLLER	0x0e
#define NOTE_LEVEL				32767

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
	envelope_instance_t	envelope_instance;
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

envelope_stage_t envelope_stages[4] =
{
	{ 0,				32767,	100, 			},
	{ 32767, 			16384,	250				},
	{ 16384,			16384,	DURATION_HELD 	},
	{ LEVEL_CURRENT,	0,		100				}
};

envelope_t envelope = { 4, envelope_stages };

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

void process_audio(int32_t timestep_ms)
{
	int write_buffer_index = alsa_lock_next_write_buffer();
	void* buffer_data;
	int buffer_samples;
	alsa_get_buffer_params(write_buffer_index, &buffer_data, &buffer_samples);
	size_t buffer_bytes = buffer_samples * sizeof(int16_t) * 2;

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
				envelope_start(&voice[i].envelope_instance);
			}

			voice[i].last_note = voice[i].current_note;
		}

		if (voice[i].current_note != -1)
		{
			int32_t envelope_level = envelope_step(&voice[i].envelope_instance, timestep_ms);
			int32_t note_level = (NOTE_LEVEL * envelope_level) / ENVELOPE_LEVEL_MAX;

			oscillator[i].level = note_level * master_level;
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
		memset(buffer_data, 0, buffer_bytes);

		gfx_event_t gfx_event;
		gfx_event.type = GFX_EVENT_SILENCE;
		gfx_event.size = buffer_bytes;
		gfx_send_event(&gfx_event);
	}
	else
	{
		gfx_event_t gfx_event;
		gfx_event.type = GFX_EVENT_WAVE;
		gfx_event.ptr = malloc(buffer_bytes);
		if (gfx_event.ptr != NULL)
		{
			gfx_event.size = buffer_bytes;
			memcpy(gfx_event.ptr, buffer_data, buffer_bytes);
			gfx_send_event(&gfx_event);
		}
	}

	alsa_unlock_buffer(write_buffer_index);
}

void tune_oscilloscope_to_note(int note)
{
	int note_wavelength = midi_get_note_wavelength_samples(note);
	gfx_wave_render_wavelength(note_wavelength);
}

void process_buffer_swap(gfx_event_t *event)
{
	static int last_note = -1;
	int oscilloscope_tuned_note = midi_get_controller_value(MIDI_CONTROL_CHANNEL, OSCILLOSCOPE_CONTROLLER);
	if (last_note != oscilloscope_tuned_note)
	{
		last_note = oscilloscope_tuned_note;
		tune_oscilloscope_to_note(last_note);
	}
}

int main(int argc, char **argv)
{
	gfx_wave_render_initialise();

	gfx_event_initialise();
	gfx_initialise();

	if (alsa_initialise("hw:1", 128) < 0)
	{
		exit(EXIT_FAILURE);
	}

	if (midi_initialise("/dev/midi2", "/dev/midi3") < 0)
	{
		exit(EXIT_FAILURE);
	}

	waveform_initialise();
	gfx_register_event_handler(GFX_EVENT_BUFFERSWAP, process_buffer_swap);

	for (int i = 0; i < VOICE_COUNT; i++)
	{
		osc_init(&oscillator[i]);
		envelope_init(&voice[i].envelope_instance, &envelope);
	}

	int profiling = 0;

	int32_t last_timestamp = get_elapsed_time_ms();

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

		int32_t timestamp = get_elapsed_time_ms();
		process_audio(timestamp - last_timestamp);
		last_timestamp = timestamp;
	}

	if (argc > 1)
	{
		ProfilerStop();
	}

	gfx_deinitialise();
	gfx_wave_render_deinitialise();
	alsa_deinitialise();
	midi_deinitialise();

	printf("Done: %d xruns\n", alsa_get_xruns_count());
	return 0;
}
