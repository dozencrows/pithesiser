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
#include <libgen.h>
#include <libconfig.h>
#include <gperftools/profiler.h>
#include "system_constants.h"
#include "fixed_point_math.h"
#include "alsa.h"
#include "midi.h"
#include "waveform.h"
#include "oscillator.h"
#include "envelope.h"
#include "voice.h"
#include "filter.h"
#include "gfx.h"
#include "gfx_event.h"
#include "gfx_event_types.h"
#include "gfx_wave_render.h"
#include "gfx_envelope_render.h"
#include "gfx_image.h"
#include "master_time.h"
#include "synth_controllers.h"
#include "mixer.h"
#include "lfo.h"
#include "recording.h"
#include "code_timing_tests.h"

//-----------------------------------------------------------------------------------------------------------------------
// Commons
//
#define WAVE_RENDERER_ID			1
#define ENVELOPE_RENDERER_ID		2
#define IMAGE_RENDERER_ID			3
#define FREQ_ENVELOPE_RENDERER_ID	4
#define Q_ENVELOPE_RENDERER_ID		5

#define EXIT_CONTROLLER			0x2e
#define PROFILE_CONTROLLER		0x2c

static const char* CFG_DEVICES_AUDIO_OUTPUT = "devices.audio.output";
static const char* CFG_DEVICES_AUDIO_AUTO_DUCK = "devices.audio.auto_duck";
static const char* RESOURCES_PITHESISER_ALPHA_PNG = "resources/pithesiser_alpha.png";
static const char* RESOURCES_SYNTH_CFG = "resources/synth.cfg";
static const char* CFG_DEVICES_MIDI_NOTE_CHANNEL = "devices.midi.note_channel";
static const char* CFG_DEVICES_MIDI_CONTROLLER_CHANNEL = "devices.midi.controller_channel";
static const char* CFG_CONTROLLERS = "controllers";
static const char* CFG_DEVICES_MIDI_INPUT = "devices.midi.input";
static const char* CFG_TESTS = "tests";
static const char* CFG_CODE_TIMING_TESTS = "code_timing";
static const char* CFG_SYSEX_INIT = "sysex.init_message";

config_t app_config;

//-----------------------------------------------------------------------------------------------------------------------
// Audio processing
//
#define VOICE_COUNT	8
voice_t voice[VOICE_COUNT];

int active_voices;

envelope_stage_t envelope_stages[4] =
{
	{ 0,				LEVEL_MAX,		100, 			},
	{ LEVEL_MAX,		LEVEL_MAX / 2,	250				},
	{ LEVEL_MAX / 2,	LEVEL_MAX / 2,	DURATION_HELD 	},
	{ LEVEL_CURRENT,	0,				100				}
};

envelope_t envelope = { LEVEL_MAX, 4, envelope_stages };

int32_t master_volume = LEVEL_MAX;
waveform_type_t master_waveform = WAVE_FIRST_AUDIBLE;

lfo_t lfo;

filter_definition_t global_filter_def;

envelope_stage_t freq_envelope_stages[4] =
{
	{ FILTER_FIXED_ONE * 20,	FILTER_FIXED_ONE * 12000,	1000, 			},
	{ FILTER_FIXED_ONE * 12000,	FILTER_FIXED_ONE * 12000,	1				},
	{ FILTER_FIXED_ONE * 12000,	FILTER_FIXED_ONE * 12000,	DURATION_HELD 	},
	{ LEVEL_CURRENT,			FILTER_FIXED_ONE * 20,		200				}
};

envelope_t freq_envelope = { FILTER_FIXED_ONE * 18000, 4, freq_envelope_stages };

envelope_stage_t q_envelope_stages[4] =
{
	{ FIXED_ONE / 100,	FIXED_ONE * .75,	1000, 			},
	{ FIXED_ONE * .75,	FIXED_ONE * .75,	1				},
	{ FIXED_ONE * .75,	FIXED_ONE * .75,	DURATION_HELD 	},
	{ LEVEL_CURRENT,	FIXED_HALF,	200				}
};

envelope_t q_envelope = { FIXED_ONE, 4, q_envelope_stages };

static int32_t duck_level_by_voice_count[] = { LEVEL_MAX, LEVEL_MAX, LEVEL_MAX * 0.65f, LEVEL_MAX * 0.49f, LEVEL_MAX * 0.40f,
											   LEVEL_MAX * 0.34f, LEVEL_MAX * 0.29f, LEVEL_MAX * 0.25f, LEVEL_MAX * 0.22f};

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

	config_setting_t *setting_auto_duck = config_lookup(&app_config, CFG_DEVICES_AUDIO_AUTO_DUCK);

	if (setting_auto_duck != NULL)
	{
		int duck_setting_count = config_setting_length(setting_auto_duck);

		if (duck_setting_count != VOICE_COUNT)
		{
			printf("Invalid number of auto duck levels %d - should be %d\n", duck_setting_count, VOICE_COUNT);
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < duck_setting_count; i++)
		{
			float duck_level_factor = config_setting_get_float_elem(setting_auto_duck, i);
			if (duck_level_factor < 0.0f || duck_level_factor > 1.0f)
			{
				printf("Invalid auto duck level %d of %f - should be between 0 and 1\n", i + 1, duck_level_factor);
				exit(EXIT_FAILURE);
			}

			int32_t duck_level = LEVEL_MAX * duck_level_factor;

			if (duck_level > duck_level_by_voice_count[i])
			{
				printf("Invalid auto duck level %d of %f - values should be decreasing\n", i + 1, duck_level_factor);
				exit(EXIT_FAILURE);
			}

			duck_level_by_voice_count[i + 1] = duck_level;
		}
	}
}

void process_audio(int32_t timestep_ms)
{
	int write_buffer_index = alsa_lock_next_write_buffer();
	void* buffer_data;
	int buffer_samples;
	alsa_get_buffer_params(write_buffer_index, &buffer_data, &buffer_samples);
	size_t buffer_bytes = buffer_samples * sizeof(sample_t) * 2;

	lfo_update(&lfo, buffer_samples);

	int first_audible_voice = -1;
	int last_active_voices = active_voices;
	int32_t auto_duck_level = duck_level_by_voice_count[active_voices];
	sample_t *voice_buffer = (sample_t*)alloca(buffer_samples * sizeof(sample_t));
	int32_t voice_level = (master_volume * auto_duck_level) / LEVEL_MAX;

	for (int i = 0; i < VOICE_COUNT; i++)
	{
		switch(voice_update(voice + i, voice_level, voice_buffer, buffer_samples, timestep_ms, &lfo, &global_filter_def))
		{
			case VOICE_IDLE:
				break;
			case VOICE_ACTIVE:
				if (first_audible_voice < 0)
				{
					first_audible_voice = i;
					//copy_mono_to_stereo(voice_buffer, PAN_MAX, PAN_MAX, buffer_samples, buffer_data);
					copy_mono_to_stereo_asm(voice_buffer, PAN_MAX, PAN_MAX, buffer_samples, buffer_data);
				}
				else
				{
					//mixdown_mono_to_stereo(voice_buffer, PAN_MAX, PAN_MAX, buffer_samples, buffer_data);
					mixdown_mono_to_stereo_asm(voice_buffer, PAN_MAX, PAN_MAX, buffer_samples, buffer_data);
				}
				break;
			case VOICE_GONE_IDLE:
				active_voices--;
				break;
		}
	}

	if (first_audible_voice < 0)
	{
		memset(buffer_data, 0, buffer_bytes);
	}

	gfx_event_t gfx_event;
	gfx_event.type = GFX_EVENT_WAVE;
	gfx_event.flags = GFX_EVENT_FLAG_OWNPTR;
	gfx_event.ptr = malloc(buffer_bytes);
	gfx_event.receiver_id = WAVE_RENDERER_ID;
	if (gfx_event.ptr != NULL)
	{
		gfx_event.size = buffer_bytes;
		memcpy(gfx_event.ptr, buffer_data, buffer_bytes);
		gfx_send_event(&gfx_event);
	}

	alsa_unlock_buffer(write_buffer_index);

	if (last_active_voices != active_voices)
	{
		if (active_voices < 0)
		{
			printf("Voice underflow: %d\n", active_voices);
		}
	}
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

	config_setting_t *setting_sysex_init_message = config_lookup(&app_config, CFG_SYSEX_INIT);
	if (setting_sysex_init_message != NULL)
	{
		int message_len = config_setting_length(setting_sysex_init_message);
		char *message_bytes = alloca(message_len);
		for (int i = 0; i < message_len; i++)
		{
			message_bytes[i] = config_setting_get_int_elem(setting_sysex_init_message, i);
		}

		midi_send_sysex(message_bytes, message_len);
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
			int candidate_voice_state;
			voice_t *candidate_voice = voice_find_next_likely_free(voice, VOICE_COUNT, &candidate_voice_state);

			if (active_voices == 0)
			{
				lfo_reset(&lfo);
			}

			if (candidate_voice_state == VOICE_IDLE)
			{
				active_voices++;
			}

			voice_play_note(candidate_voice, midi_event.data[0], master_waveform);
		}
		else if (midi_event.type == 0x80)
		{
			voice_t *playing_voice = voice_find_playing_note(voice, VOICE_COUNT, midi_event.data[0]);
			if (playing_voice != NULL)
			{
				voice_stop_note(playing_voice);
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
		gfx_event.flags = 0;
		gfx_event.receiver_id = ENVELOPE_RENDERER_ID;
		gfx_send_event(&gfx_event);
	}

	if (midi_controller_update(&lfo_state_controller, &param_value))
	{
		lfo.state = param_value;
	}

	if (midi_controller_update(&lfo_waveform_controller, &param_value))
	{
		lfo.oscillator.waveform = param_value;
	}

	if (midi_controller_update(&lfo_level_controller, &param_value))
	{
		lfo.oscillator.level = param_value;
	}

	if (midi_controller_update(&lfo_frequency_controller, &param_value))
	{
		lfo.oscillator.frequency = param_value;
	}

	if (midi_controller_update(&filter_state_controller, &param_value))
	{
		global_filter_def.type = param_value;
	}

	if (midi_controller_update(&filter_frequency_controller, &param_value))
	{
		global_filter_def.frequency = param_value;
	}

	if (midi_controller_update(&filter_q_controller, &param_value))
	{
		global_filter_def.q = param_value;
	}
}

//-----------------------------------------------------------------------------------------------------------------------
// UI data & management
//

wave_renderer_t *waveform_renderer = NULL;
envelope_renderer_t *envelope_renderer = NULL;
image_renderer_t *image_renderer = NULL;
envelope_renderer_t *freq_envelope_renderer = NULL;
envelope_renderer_t *q_envelope_renderer = NULL;

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
	envelope_renderer->width = 510;
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
	envelope_renderer->text = "amplitude env";
	envelope_renderer->text_colour[0] = 255.0f;
	envelope_renderer->text_colour[1] = 255.0f;
	envelope_renderer->text_colour[2] = 255.0f;
	envelope_renderer->text_colour[3] = 255.0f;

	freq_envelope_renderer = gfx_envelope_renderer_create(FREQ_ENVELOPE_RENDERER_ID);

	freq_envelope_renderer->x = 512;
	freq_envelope_renderer->y = 514;
	freq_envelope_renderer->width = 512;
	freq_envelope_renderer->height = 119;
	freq_envelope_renderer->envelope = &freq_envelope;
	freq_envelope_renderer->background_colour[0] = 0.0f;
	freq_envelope_renderer->background_colour[1] = 0.0f;
	freq_envelope_renderer->background_colour[2] = 16.0f;
	freq_envelope_renderer->background_colour[3] = 255.0f;
	freq_envelope_renderer->line_colour[0] = 0.0f;
	freq_envelope_renderer->line_colour[1] = 255.0f;
	freq_envelope_renderer->line_colour[2] = 0.0f;
	freq_envelope_renderer->line_colour[3] = 255.0f;
	freq_envelope_renderer->text = "filter freq env";
	freq_envelope_renderer->text_colour[0] = 255.0f;
	freq_envelope_renderer->text_colour[1] = 255.0f;
	freq_envelope_renderer->text_colour[2] = 255.0f;
	freq_envelope_renderer->text_colour[3] = 255.0f;

	q_envelope_renderer = gfx_envelope_renderer_create(Q_ENVELOPE_RENDERER_ID);

	q_envelope_renderer->x = 512;
	q_envelope_renderer->y = 634;
	q_envelope_renderer->width = 512;
	q_envelope_renderer->height = 120;
	q_envelope_renderer->envelope = &q_envelope;
	q_envelope_renderer->background_colour[0] = 0.0f;
	q_envelope_renderer->background_colour[1] = 0.0f;
	q_envelope_renderer->background_colour[2] = 16.0f;
	q_envelope_renderer->background_colour[3] = 255.0f;
	q_envelope_renderer->line_colour[0] = 0.0f;
	q_envelope_renderer->line_colour[1] = 255.0f;
	q_envelope_renderer->line_colour[2] = 0.0f;
	q_envelope_renderer->line_colour[3] = 255.0f;
	q_envelope_renderer->text = "filter Q env";
	q_envelope_renderer->text_colour[0] = 255.0f;
	q_envelope_renderer->text_colour[1] = 255.0f;
	q_envelope_renderer->text_colour[2] = 255.0f;
	q_envelope_renderer->text_colour[3] = 255.0f;

	image_renderer = gfx_image_renderer_create(IMAGE_RENDERER_ID);
	image_renderer->x = 0;
	image_renderer->y = 0;
	image_renderer->width = 157;
	image_renderer->height = 140;
	image_renderer->image_file = (char*)RESOURCES_PITHESISER_ALPHA_PNG;
}

void tune_oscilloscope_to_note(int note)
{
	int note_wavelength = midi_get_note_wavelength_samples(note);
	gfx_wave_render_wavelength(waveform_renderer, note_wavelength);
}

void destroy_ui()
{
	gfx_image_renderer_destroy(image_renderer);
	gfx_envelope_renderer_destroy(q_envelope_renderer);
	gfx_envelope_renderer_destroy(freq_envelope_renderer);
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
// Synth setup and main loop
//
const char *profile_file = NULL;

void configure_profiling()
{
	config_lookup_string(&app_config, "profiling.output_file", &profile_file);
}

void synth_main()
{
	gfx_wave_render_initialise();
	gfx_envelope_render_initialise();

	create_ui();

	gfx_event_initialise();
	gfx_initialise();

	configure_audio();
	configure_midi();
	configure_profiling();

	waveform_initialise();
	gfx_register_event_global_handler(GFX_EVENT_BUFFERSWAP, process_buffer_swap);

	voice_init(voice, VOICE_COUNT, &envelope, &freq_envelope, &q_envelope);
	active_voices = 0;

	lfo_init(&lfo);

	global_filter_def.type = FILTER_PASS;
	global_filter_def.frequency = 9000 * FILTER_FIXED_ONE;
	global_filter_def.q = FIXED_HALF;

	int profiling = 0;

	int32_t last_timestamp = get_elapsed_time_ms();

	while (1)
	{
		int midi_controller_value;

		if (midi_controller_update(&exit_controller, &midi_controller_value))
		{
			break;
		}

		process_midi_events();

		if (!profiling && midi_controller_update(&profile_controller, &midi_controller_value))
		{
			if (profile_file != NULL)
			{
				ProfilerStart(profile_file);
				profiling = 1;
			}
		}

		alsa_sync_with_audio_output();

		int32_t timestamp = get_elapsed_time_ms();
		process_audio(timestamp - last_timestamp);
		last_timestamp = timestamp;
	}

	if (profiling)
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
}

//-----------------------------------------------------------------------------------------------------------------------
// Entrypoint
//
int main(int argc, char **argv)
{
	const char* config_file = RESOURCES_SYNTH_CFG;
	if (argc > 1)
	{
		config_file = argv[1];
	}

	char config_dir[PATH_MAX];
	realpath(config_file, config_dir);
	dirname(config_dir);
	config_init(&app_config);
	config_set_include_dir(&app_config, config_dir);
	if (config_read_file(&app_config, config_file) != CONFIG_TRUE)
	{
		printf("Config error in %s at line %d: %s\n", config_error_file(&app_config), config_error_line(&app_config), config_error_text(&app_config));
		exit(EXIT_FAILURE);
	}

	config_setting_t *setting_tests = config_lookup(&app_config, CFG_TESTS);

	if (setting_tests != NULL)
	{
		config_setting_t *setting_code_timing_tests = config_setting_get_member(setting_tests, CFG_CODE_TIMING_TESTS);
		if (setting_code_timing_tests != NULL)
		{
			code_timing_tests_main(setting_code_timing_tests);
		}
	}
	else
	{
		recording_initialise(&app_config, WAVE_RENDERER_ID);
		synth_main();
		recording_deinitialise();
	}

	return 0;
}
