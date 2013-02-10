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
#include <libconfig.h>
#include <gperftools/profiler.h>
#include "system_constants.h"
#include "alsa.h"
#include "midi.h"
#include "waveform.h"
#include "oscillator.h"
#include "envelope.h"
#include "filter.h"
#include "gfx.h"
#include "gfx_event.h"
#include "gfx_event_types.h"
#include "gfx_wave_render.h"
#include "gfx_envelope_render.h"
#include "gfx_image.h"
#include "master_time.h"

//-----------------------------------------------------------------------------------------------------------------------
// Commons
//
#define WAVE_RENDERER_ID		1
#define ENVELOPE_RENDERER_ID	2
#define IMAGE_RENDERER_ID		3

config_t app_config;

//-----------------------------------------------------------------------------------------------------------------------
// Audio processing
//
#define MIN_FREQUENCY			55.0f
#define MAX_FREQUENCY			1760.0f
#define NOTE_LEVEL				32767
#define NOTE_ENDING				-2
#define NOTE_NOT_PLAYING		-1

#define LFO_MIN_FREQUENCY		(FIXED_ONE / 10)
#define LFO_MAX_FREQUENCY		(20 * FIXED_ONE)

#define LFO_STATE_OFF			0
#define LFO_STATE_VOLUME		1
#define LFO_STATE_PITCH			2
#define LFO_STATE_LAST			LFO_STATE_PITCH

#define FILTER_STATE_OFF		FILTER_PASS
#define FILTER_STATE_LAST		FILTER_HPF
#define FILTER_MIN_FREQUENCY	(FIXED_ONE / 10)
#define FILTER_MAX_FREQUENCY	(4186 * FIXED_ONE)
#define FILTER_MIN_Q			(FIXED_ONE / 100)
#define FILTER_MAX_Q			(FIXED_ONE)

typedef struct
{
	int	midi_channel;
	int last_note;
	int current_note;
	int play_counter;
	envelope_instance_t	envelope_instance;
} voice_t;

#define VOICE_COUNT	8
voice_t voice[VOICE_COUNT] =
{
	{ 0, NOTE_NOT_PLAYING, NOTE_NOT_PLAYING, 0 },
	{ 0, NOTE_NOT_PLAYING, NOTE_NOT_PLAYING, 0 },
	{ 0, NOTE_NOT_PLAYING, NOTE_NOT_PLAYING, 0 },
	{ 0, NOTE_NOT_PLAYING, NOTE_NOT_PLAYING, 0 },
	{ 0, NOTE_NOT_PLAYING, NOTE_NOT_PLAYING, 0 },
	{ 0, NOTE_NOT_PLAYING, NOTE_NOT_PLAYING, 0 },
	{ 0, NOTE_NOT_PLAYING, NOTE_NOT_PLAYING, 0 },
	{ 0, NOTE_NOT_PLAYING, NOTE_NOT_PLAYING, 0 },
};

oscillator_t oscillator[VOICE_COUNT];
int active_voices;

envelope_stage_t envelope_stages[4] =
{
	{ 0,				32767,	100, 			},
	{ 32767, 			16384,	250				},
	{ 16384,			16384,	DURATION_HELD 	},
	{ LEVEL_CURRENT,	0,		100				}
};

envelope_t envelope = { 4, envelope_stages };

float master_level = 1.0f;
waveform_type_t master_waveform = WAVE_FIRST_AUDIBLE;

oscillator_t lf_oscillator;
int lfo_state;

filter_t global_filter;

void configure_audio()
{
	const char *setting_devices_audio_output = NULL;

	if (config_lookup_string(&app_config, "devices.audio.output", &setting_devices_audio_output) != CONFIG_TRUE)
	{
		printf("Missing audio output device in config\n");
		exit(EXIT_FAILURE);
	}

	if (alsa_initialise(setting_devices_audio_output, 128) < 0)
	{
		exit(EXIT_FAILURE);
	}
}

void process_audio(int32_t timestep_ms)
{
	int write_buffer_index = alsa_lock_next_write_buffer();
	void* buffer_data;
	int buffer_samples;
	alsa_get_buffer_params(write_buffer_index, &buffer_data, &buffer_samples);
	size_t buffer_bytes = buffer_samples * sizeof(sample_t) * 2;
	sample_t lfo_value = 0;

	if (lfo_state)
	{
		osc_mid_output(&lf_oscillator, buffer_samples, &lfo_value);
	}

	int first_audible_voice = -1;

	for (int i = 0; i < VOICE_COUNT; i++)
	{
		if (voice[i].current_note != voice[i].last_note)
		{
			if (voice[i].current_note == NOTE_NOT_PLAYING)
			{
				oscillator[i].level = 0;
			}
			else if (voice[i].current_note >= 0)
			{
				oscillator[i].frequency = midi_get_note_frequency(voice[i].current_note);
				oscillator[i].waveform = master_waveform;
				oscillator[i].phase_accumulator = 0;
				envelope_start(&voice[i].envelope_instance);
			}

			voice[i].last_note = voice[i].current_note;
		}

		if (voice[i].current_note != NOTE_NOT_PLAYING)
		{
			int32_t envelope_level = envelope_step(&voice[i].envelope_instance, timestep_ms);
			int32_t note_level = (NOTE_LEVEL * envelope_level) / ENVELOPE_LEVEL_MAX;

			fixed_t base_frequency = oscillator[i].frequency;

			if (lfo_state == LFO_STATE_VOLUME)
			{
				note_level = (note_level * lfo_value) / SHRT_MAX;
			}
			else if (lfo_state == LFO_STATE_PITCH)
			{
				// TODO: use a proper fixed point power function!
				oscillator[i].frequency = (((int64_t)base_frequency * (int64_t)(powf(2.0f, (float)lfo_value / (float)SHRT_MAX) * FIXED_ONE)) >> FIXED_PRECISION);
			}

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
			else if (voice[i].current_note == NOTE_ENDING)
			{
				voice[i].current_note = NOTE_NOT_PLAYING;
				active_voices--;
			}

			if (lfo_state == LFO_STATE_PITCH)
			{
				oscillator[i].frequency = base_frequency;
			}
		}
	}

	filter_apply(&global_filter, buffer_samples, buffer_data);

	if (first_audible_voice < 0)
	{
		memset(buffer_data, 0, buffer_bytes);

		gfx_event_t gfx_event;
		gfx_event.type = GFX_EVENT_SILENCE;
		gfx_event.size = buffer_bytes;
		gfx_event.receiver_id = WAVE_RENDERER_ID;
		gfx_send_event(&gfx_event);
	}
	else
	{
		gfx_event_t gfx_event;
		gfx_event.type = GFX_EVENT_WAVE;
		gfx_event.ptr = malloc(buffer_bytes);
		gfx_event.receiver_id = WAVE_RENDERER_ID;
		if (gfx_event.ptr != NULL)
		{
			gfx_event.size = buffer_bytes;
			memcpy(gfx_event.ptr, buffer_data, buffer_bytes);
			gfx_send_event(&gfx_event);
		}
	}

	alsa_unlock_buffer(write_buffer_index);
}

//-----------------------------------------------------------------------------------------------------------------------
// Midi processing
//
#define WAVE_CONTROLLER			0x21
#define EXIT_CONTROLLER			0x2e
#define PROFILE_CONTROLLER		0x2c
#define OSCILLOSCOPE_CONTROLLER	0x0e
#define VOLUME_CONTROLLER		0x02

#define ENVELOPE_ATTACK_LEVEL_CTRL	0x03	// slider 2
#define ENVELOPE_ATTACK_TIME_CTRL	0x0f	// dial 2
#define ENVELOPE_DECAY_LEVEL_CTRL	0x04	// slider 3
#define ENVELOPE_DECAY_TIME_CTRL	0x10	// dial 3
#define ENVELOPE_SUSTAIN_TIME_CTRL	0x11	// dial 4
#define ENVELOPE_RELEASE_TIME_CTRL	0x05	// slider 4

#define LFO_TOGGLE					0x1b	// upper switch 5
#define LFO_WAVEFORM				0x25	// lower switch 5
#define LFO_FREQUENCY				0x12	// dial 5
#define LFO_LEVEL					0x06	// slider 5

#define ENVELOPE_ATTACK_DURATION_SCALE	10
#define ENVELOPE_DECAY_DURATION_SCALE	10
#define ENVELOPE_SUSTAIN_DURATION_SCALE	10
#define ENVELOPE_RELEASE_DURATION_SCALE	10

#define FILTER_TOGGLE				0x1c	// upper switch 6
#define FILTER_FREQUENCY			0x13	// dial 6
#define FILTER_Q					0x08	// slider 6

int controller_channel = 0;

void configure_midi()
{
	int note_channel = 0;

	config_lookup_int(&app_config, "devices.midi.controller_channel", &controller_channel);
	config_lookup_int(&app_config, "devices.midi.note_channel", &note_channel);

	for (int i = 0 ; i < VOICE_COUNT; i++)
	{
		voice[i].midi_channel = note_channel;
	}

	config_setting_t *setting_devices_midi_input = config_lookup(&app_config, "devices.midi.input");

	if (setting_devices_midi_input == NULL)
	{
		printf("Missing midi input devices in config\n");
		exit(EXIT_FAILURE);
	}

	const char* midi_device_names[MAX_MIDI_DEVICES];
	int midi_device_count = config_setting_length(setting_devices_midi_input);

	if (midi_device_count < 0 || midi_device_count > MAX_MIDI_DEVICES)
	{
		printf("Invalid number of midi devices %d - should be between 1 and %d\n", midi_device_count, MAX_MIDI_DEVICES);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < midi_device_count; i++)
	{
		midi_device_names[i] = config_setting_get_string_elem(setting_devices_midi_input, i);
	}

	if (midi_initialise(midi_device_count, midi_device_names) < 0)
	{
		exit(EXIT_FAILURE);
	}
}

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
				if (voice[i].current_note == NOTE_NOT_PLAYING)
				{
					candidate_voice = i;

					if (active_voices == 0)
					{
						lf_oscillator.phase_accumulator = 0;
					}
					active_voices++;
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
					voice[i].current_note = NOTE_ENDING;
					envelope_go_to_stage(&voice[i].envelope_instance, ENVELOPE_STAGE_RELEASE);
				}
			}
		}
	}
}

extern void tune_oscilloscope_to_note(int note);

void process_midi_controllers()
{
	master_level = (float) midi_get_controller_value(controller_channel, VOLUME_CONTROLLER) / MIDI_MAX_CONTROLLER_VALUE;

	if (midi_get_controller_changed(controller_channel, WAVE_CONTROLLER))
	{
		if (midi_get_controller_value(controller_channel, WAVE_CONTROLLER) > 63)
		{
			master_waveform++;
			if (master_waveform > WAVE_LAST_AUDIBLE)
			{
				master_waveform = WAVE_FIRST_AUDIBLE;
			}
		}
	}

	static int last_note = -1;
	int oscilloscope_tuned_note = midi_get_controller_value(controller_channel, OSCILLOSCOPE_CONTROLLER);
	if (last_note != oscilloscope_tuned_note)
	{
		last_note = oscilloscope_tuned_note;
		tune_oscilloscope_to_note(last_note);
	}

	int envelope_updated = 0;

	// Suggested data driven model:
	//	Parameters: controller id, value calculation func & params, destination values.
	if (midi_get_controller_changed(controller_channel, ENVELOPE_ATTACK_LEVEL_CTRL))
	{
		int value = midi_get_controller_value(controller_channel, ENVELOPE_ATTACK_LEVEL_CTRL);
		int32_t envelope_level = (ENVELOPE_LEVEL_MAX * value) / MIDI_MAX_CONTROLLER_VALUE;
		envelope.stages[ENVELOPE_STAGE_ATTACK].end_level = envelope_level;
		envelope.stages[ENVELOPE_STAGE_DECAY].start_level = envelope_level;
		envelope_updated = 1;
	}

	if (midi_get_controller_changed(controller_channel, ENVELOPE_ATTACK_TIME_CTRL))
	{
		int value = midi_get_controller_value(controller_channel, ENVELOPE_ATTACK_TIME_CTRL);
		envelope.stages[ENVELOPE_STAGE_ATTACK].duration = value * ENVELOPE_ATTACK_DURATION_SCALE;
		envelope_updated = 1;
	}

	if (midi_get_controller_changed(controller_channel, ENVELOPE_DECAY_LEVEL_CTRL))
	{
		int value = midi_get_controller_value(controller_channel, ENVELOPE_DECAY_LEVEL_CTRL);
		int32_t envelope_level = (ENVELOPE_LEVEL_MAX * value) / MIDI_MAX_CONTROLLER_VALUE;
		envelope.stages[ENVELOPE_STAGE_DECAY].end_level = envelope_level;
		envelope.stages[ENVELOPE_STAGE_SUSTAIN].start_level = envelope_level;
		envelope.stages[ENVELOPE_STAGE_SUSTAIN].end_level = envelope_level;
		envelope_updated = 1;
	}

	if (midi_get_controller_changed(controller_channel, ENVELOPE_DECAY_TIME_CTRL))
	{
		int value = midi_get_controller_value(controller_channel, ENVELOPE_DECAY_TIME_CTRL);
		envelope.stages[ENVELOPE_STAGE_DECAY].duration = value * ENVELOPE_DECAY_DURATION_SCALE;
		envelope_updated = 1;
	}

	if (midi_get_controller_changed(controller_channel, ENVELOPE_SUSTAIN_TIME_CTRL))
	{
		int value = midi_get_controller_value(controller_channel, ENVELOPE_SUSTAIN_TIME_CTRL);
		if (value < MIDI_MAX_CONTROLLER_VALUE)
		{
			envelope.stages[ENVELOPE_STAGE_SUSTAIN].duration = value * ENVELOPE_SUSTAIN_DURATION_SCALE;
		}
		else
		{
			envelope.stages[ENVELOPE_STAGE_SUSTAIN].duration = DURATION_HELD;
		}
		envelope_updated = 1;
	}

	if (midi_get_controller_changed(controller_channel, ENVELOPE_RELEASE_TIME_CTRL))
	{
		int value = midi_get_controller_value(controller_channel, ENVELOPE_RELEASE_TIME_CTRL);
		envelope.stages[ENVELOPE_STAGE_RELEASE].duration = value * ENVELOPE_RELEASE_DURATION_SCALE;
		envelope_updated = 1;
	}

	if (envelope_updated)
	{
		gfx_event_t gfx_event;
		gfx_event.type = GFX_EVENT_REFRESH;
		gfx_event.receiver_id = ENVELOPE_RENDERER_ID;
		gfx_send_event(&gfx_event);
	}

	if (midi_get_controller_changed(controller_channel, LFO_TOGGLE))
	{
		if (midi_get_controller_value(controller_channel, LFO_TOGGLE) > MIDI_MID_CONTROLLER_VALUE)
		{
			lfo_state++;
			if (lfo_state > LFO_STATE_LAST)
			{
				lfo_state = LFO_STATE_OFF;
			}
		}
	}

	if (midi_get_controller_changed(controller_channel, LFO_WAVEFORM))
	{
		if (midi_get_controller_value(controller_channel, LFO_WAVEFORM) > MIDI_MID_CONTROLLER_VALUE)
		{
			lf_oscillator.waveform++;
			if (lf_oscillator.waveform > WAVE_LAST_LFO)
			{
				lf_oscillator.waveform = WAVE_FIRST_LFO;
			}
		}
	}

	if (midi_get_controller_changed(controller_channel, LFO_LEVEL))
	{
		int value = midi_get_controller_value(controller_channel, LFO_LEVEL);
		lf_oscillator.level = (SHRT_MAX * value) / MIDI_MAX_CONTROLLER_VALUE;
	}

	if (midi_get_controller_changed(controller_channel, LFO_FREQUENCY))
	{
		int value = midi_get_controller_value(controller_channel, LFO_FREQUENCY);
		lf_oscillator.frequency = LFO_MIN_FREQUENCY + ((LFO_MAX_FREQUENCY - LFO_MIN_FREQUENCY) * value) / MIDI_MAX_CONTROLLER_VALUE;
	}

	int filter_changed = 0;

	if (midi_get_controller_changed(controller_channel, FILTER_TOGGLE))
	{
		if (midi_get_controller_value(controller_channel, FILTER_TOGGLE) > MIDI_MID_CONTROLLER_VALUE)
		{
			global_filter.definition.type++;;
			if (global_filter.definition.type > FILTER_STATE_LAST)
			{
				global_filter.definition.type = FILTER_STATE_OFF;
			}
			filter_changed++;
		}
	}

	if (midi_get_controller_changed(controller_channel, FILTER_FREQUENCY))
	{
		int value = midi_get_controller_value(controller_channel, FILTER_FREQUENCY);
		fixed_wide_t frequency_offset = ((fixed_wide_t)(FILTER_MAX_FREQUENCY - FILTER_MIN_FREQUENCY) * (fixed_wide_t)value) / (fixed_wide_t)MIDI_MAX_CONTROLLER_VALUE;
		global_filter.definition.frequency = FILTER_MIN_FREQUENCY + frequency_offset;
		filter_changed++;
	}

	if (midi_get_controller_changed(controller_channel, FILTER_Q))
	{
		int value = midi_get_controller_value(controller_channel, FILTER_Q);
		global_filter.definition.q = FILTER_MIN_Q + ((FILTER_MAX_Q - FILTER_MIN_Q) * value) / MIDI_MAX_CONTROLLER_VALUE;
		filter_changed++;
	}

	if (filter_changed)
	{
		filter_update(&global_filter);
	}
}

//-----------------------------------------------------------------------------------------------------------------------
// UI data & management
//

wave_renderer_t *waveform_renderer = NULL;
envelope_renderer_t *envelope_renderer = NULL;
image_renderer_t *image_renderer = NULL;

void process_postinit_ui(gfx_event_t *event, gfx_object_t *receiver)
{
	int screen_width;
	int screen_height;

	gfx_get_screen_resolution(&screen_width, &screen_height);

	image_renderer->y = screen_height - image_renderer->height;
}

void create_ui()
{
	gfx_register_event_global_handler(GFX_EVENT_POSTINIT, process_postinit_ui);

	waveform_renderer = gfx_wave_renderer_create(WAVE_RENDERER_ID);

	waveform_renderer->x = 0;
	waveform_renderer->y = 0;
	waveform_renderer->width = 1024;
	waveform_renderer->height = 512;
	waveform_renderer->amplitude_scale = 129;
	waveform_renderer->tuned_wavelength_fx = 129;
	waveform_renderer->background_colour[0] = 0.0f;
	waveform_renderer->background_colour[1] = 0.0f;
	waveform_renderer->background_colour[2] = 16.0f;
	waveform_renderer->background_colour[3] = 255.0f;
	waveform_renderer->line_colour[0] = 0.0f;
	waveform_renderer->line_colour[1] = 255.0f;
	waveform_renderer->line_colour[2] = 0.0f;
	waveform_renderer->line_colour[3] = 255.0f;

	envelope_renderer = gfx_envelope_renderer_create(ENVELOPE_RENDERER_ID);

	envelope_renderer->x = 0;
	envelope_renderer->y = 514;
	envelope_renderer->width = 512;
	envelope_renderer->height = 240;
	envelope_renderer->envelope = &envelope;
	envelope_renderer->background_colour[0] = 0.0f;
	envelope_renderer->background_colour[1] = 0.0f;
	envelope_renderer->background_colour[2] = 16.0f;
	envelope_renderer->background_colour[3] = 255.0f;
	envelope_renderer->line_colour[0] = 0.0f;
	envelope_renderer->line_colour[1] = 255.0f;
	envelope_renderer->line_colour[2] = 0.0f;
	envelope_renderer->line_colour[3] = 255.0f;

	image_renderer = gfx_image_renderer_create(IMAGE_RENDERER_ID);
	image_renderer->x = 0;
	image_renderer->y = 0;
	image_renderer->width = 157;
	image_renderer->height = 140;
	image_renderer->image_file = "resources/pithesiser_alpha.png";
}

void tune_oscilloscope_to_note(int note)
{
	int note_wavelength = midi_get_note_wavelength_samples(note);
	gfx_wave_render_wavelength(waveform_renderer, note_wavelength);
}

void destroy_ui()
{
	gfx_image_renderer_destroy(image_renderer);
	gfx_envelope_renderer_destroy(envelope_renderer);
	gfx_wave_renderer_destroy(waveform_renderer);
}

//-----------------------------------------------------------------------------------------------------------------------
// GFX event handlers
//
void process_buffer_swap(gfx_event_t *event, gfx_object_t *receiver)
{
	process_midi_controllers();
}

//-----------------------------------------------------------------------------------------------------------------------
// Entrypoint
//
int main(int argc, char **argv)
{
	config_init(&app_config);
	if (config_read_file(&app_config, "resources/synth.cfg") != CONFIG_TRUE)
	{
		printf("Config error in %s at line %d:\n", config_error_file(&app_config), config_error_line(&app_config));
		printf("%s\n", config_error_text(&app_config));
		exit(EXIT_FAILURE);
	}

	gfx_wave_render_initialise();
	gfx_envelope_render_initialise();

	create_ui();

	gfx_event_initialise();
	gfx_initialise();

	configure_audio();
	configure_midi();

	waveform_initialise();
	gfx_register_event_global_handler(GFX_EVENT_BUFFERSWAP, process_buffer_swap);

	for (int i = 0; i < VOICE_COUNT; i++)
	{
		osc_init(&oscillator[i]);
		envelope_init(&voice[i].envelope_instance, &envelope);
	}
	active_voices = 0;

	osc_init(&lf_oscillator);
	lf_oscillator.waveform = LFO_PROCEDURAL_SINE;
	lf_oscillator.frequency = 1 * FIXED_ONE;
	lf_oscillator.level = SHRT_MAX;
	lfo_state = 0;

	filter_init(&global_filter);
	global_filter.definition.type = FILTER_PASS;
	global_filter.definition.frequency = midi_get_note_frequency(81);
	global_filter.definition.q = FIXED_HALF;
	filter_update(&global_filter);

	int profiling = 0;

	int32_t last_timestamp = get_elapsed_time_ms();

	while (1)
	{
		if (midi_get_controller_value(controller_channel, EXIT_CONTROLLER) > 63)
		{
			break;
		}

		process_midi_events();

		if (!profiling && midi_get_controller_changed(controller_channel, PROFILE_CONTROLLER))
		{
			if (argc > 1 && midi_get_controller_value(controller_channel, PROFILE_CONTROLLER) > 63)
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
	destroy_ui();
	gfx_envelope_render_deinitialise();
	gfx_wave_render_deinitialise();
	alsa_deinitialise();
	midi_deinitialise();
	config_destroy(&app_config);

	printf("Done: %d xruns\n", alsa_get_xruns_count());
	return 0;
}
