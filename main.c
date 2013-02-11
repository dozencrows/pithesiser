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
#include "synth_controllers.h"

//-----------------------------------------------------------------------------------------------------------------------
// Commons
//
#define WAVE_RENDERER_ID		1
#define ENVELOPE_RENDERER_ID	2
#define IMAGE_RENDERER_ID		3

#define EXIT_CONTROLLER			0x2e
#define PROFILE_CONTROLLER		0x2c

static const char* CFG_DEVICES_AUDIO_OUTPUT = "devices.audio.output";
static const char* RESOURCES_PITHESISER_ALPHA_PNG = "resources/pithesiser_alpha.png";
static const char* RESOURCES_SYNTH_CFG = "resources/synth.cfg";
static const char* CFG_DEVICES_MIDI_NOTE_CHANNEL = "devices.midi.note_channel";
static const char* CFG_DEVICES_MIDI_CONTROLLER_CHANNEL = "devices.midi.controller_channel";
static const char* CFG_CONTROLLERS = "controllers";
static const char* CFG_DEVICES_MIDI_INPUT = "devices.midi.input";

config_t app_config;

//-----------------------------------------------------------------------------------------------------------------------
// Audio processing
//
#define NOTE_ENDING				-2
#define NOTE_NOT_PLAYING		-1

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

int32_t master_volume = LEVEL_MAX;
waveform_type_t master_waveform = WAVE_FIRST_AUDIBLE;

oscillator_t lf_oscillator;
int lfo_state;

filter_t global_filter;

void configure_audio()
{
	const char *setting_devices_audio_output = NULL;

	if (config_lookup_string(&app_config, CFG_DEVICES_AUDIO_OUTPUT, &setting_devices_audio_output) != CONFIG_TRUE)
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
			int32_t note_level = (LEVEL_MAX * envelope_level) / ENVELOPE_LEVEL_MAX;

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

			oscillator[i].level = (note_level * master_volume) / LEVEL_MAX;
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
int controller_channel = 0;

void configure_midi()
{
	int note_channel = 0;

	config_lookup_int(&app_config, CFG_DEVICES_MIDI_CONTROLLER_CHANNEL, &controller_channel);
	config_lookup_int(&app_config, CFG_DEVICES_MIDI_NOTE_CHANNEL, &note_channel);

	for (int i = 0 ; i < VOICE_COUNT; i++)
	{
		voice[i].midi_channel = note_channel;
	}

	config_setting_t *setting_devices_midi_input = config_lookup(&app_config, CFG_DEVICES_MIDI_INPUT);

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

	if (!synth_controllers_initialise(controller_channel, config_lookup(&app_config, CFG_CONTROLLERS)))
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
	int param_value;

	midi_controller_update(&master_volume_controller, &master_volume);
	if (midi_controller_update(&waveform_controller, &param_value))
	{
		master_waveform = param_value;
	}

	if (midi_controller_update(&oscilloscope_controller, &param_value))
	{
		tune_oscilloscope_to_note(param_value);
	}

	int envelope_updated = 0;

	if (midi_controller_update(&envelope_attack_level_controller, &param_value))
	{
		envelope.stages[ENVELOPE_STAGE_ATTACK].end_level = param_value;
		envelope.stages[ENVELOPE_STAGE_DECAY].start_level = param_value;
		envelope_updated = 1;
	}

	if (midi_controller_update(&envelope_attack_time_controller, &param_value))
	{
		envelope.stages[ENVELOPE_STAGE_ATTACK].duration = param_value;
		envelope_updated = 1;
	}

	if (midi_controller_update(&envelope_decay_level_controller, &param_value))
	{
		envelope.stages[ENVELOPE_STAGE_DECAY].end_level = param_value;
		envelope.stages[ENVELOPE_STAGE_SUSTAIN].start_level = param_value;
		envelope.stages[ENVELOPE_STAGE_SUSTAIN].end_level = param_value;
		envelope_updated = 1;
	}

	if (midi_controller_update(&envelope_decay_time_controller, &param_value))
	{
		envelope.stages[ENVELOPE_STAGE_DECAY].duration = param_value;
		envelope_updated = 1;
	}

	if (midi_controller_update(&envelope_sustain_time_controller, &param_value))
	{
		envelope.stages[ENVELOPE_STAGE_SUSTAIN].duration = param_value;
		envelope_updated = 1;
	}

	if (midi_controller_update(&envelope_release_time_controller, &param_value))
	{
		envelope.stages[ENVELOPE_STAGE_RELEASE].duration = param_value;
		envelope_updated = 1;
	}

	if (envelope_updated)
	{
		gfx_event_t gfx_event;
		gfx_event.type = GFX_EVENT_REFRESH;
		gfx_event.receiver_id = ENVELOPE_RENDERER_ID;
		gfx_send_event(&gfx_event);
	}

	if (midi_controller_update(&lfo_state_controller, &param_value))
	{
		lfo_state = param_value;
	}

	if (midi_controller_update(&lfo_waveform_controller, &param_value))
	{
		lf_oscillator.waveform = param_value;
	}

	if (midi_controller_update(&lfo_level_controller, &param_value))
	{
		lf_oscillator.level = param_value;
	}

	if (midi_controller_update(&lfo_frequency_controller, &param_value))
	{
		lf_oscillator.frequency = param_value;
	}

	int filter_changed = 0;

	if (midi_controller_update(&filter_state_controller, &param_value))
	{
		global_filter.definition.type = param_value;
		filter_changed++;
	}

	if (midi_controller_update(&filter_frequency_controller, &param_value))
	{
		global_filter.definition.frequency = param_value;
		filter_changed++;
	}

	if (midi_controller_update(&filter_q_controller, &param_value))
	{
		global_filter.definition.q = param_value;
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
	image_renderer->image_file = RESOURCES_PITHESISER_ALPHA_PNG;
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
	if (config_read_file(&app_config, RESOURCES_SYNTH_CFG) != CONFIG_TRUE)
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
		if (midi_get_raw_controller_value(controller_channel, EXIT_CONTROLLER) > 63)
		{
			break;
		}

		process_midi_events();

		if (!profiling && midi_get_raw_controller_changed(controller_channel, PROFILE_CONTROLLER))
		{
			if (argc > 1 && midi_get_raw_controller_value(controller_channel, PROFILE_CONTROLLER) > 63)
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
