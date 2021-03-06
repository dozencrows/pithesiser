// Pithesiser - a software synthesiser for Raspberry Pi
// Copyright (C) 2015 Nicholas Tuckett
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

/*
 * synth_controllers.c
 *
 *  Created on: 11 Feb 2013
 *      Author: ntuckett
 */

#include "synth_controllers.h"
#include <string.h>
#include <stdio.h>
#include "libconfig.h"
#include "system_constants.h"
#include "logging.h"
#include "error_handler.h"
#include "midi.h"
#include "midi_controller.h"
#include "gfx_event.h"
#include "gfx_event_types.h"
#include "synth_model.h"
#include "midi_controller_parser.h"

#define ENVELOPE_TIME_SCALE		10

typedef struct envelope_controller_t
{
	int renderer_id;
	midi_controller_t attack_time_controller;
	midi_controller_t attack_level_controller;
	midi_controller_t decay_time_controller;
	midi_controller_t decay_level_controller;
	midi_controller_t sustain_time_controller;
	midi_controller_t release_time_controller;
} envelope_controller_t;

midi_controller_t master_volume_controller;
midi_controller_t waveform_controller;
midi_controller_t oscilloscope_controller;

envelope_controller_t envelope_controller[SYNTH_ENVELOPE_COUNT];

midi_controller_t lfo_waveform_controller;
midi_controller_t lfo_level_controller;
midi_controller_t lfo_frequency_controller;

midi_controller_t filter_state_controller;
midi_controller_t filter_frequency_controller;
midi_controller_t filter_q_controller;

midi_controller_t exit_controller;
midi_controller_t profile_controller;
midi_controller_t screenshot_controller;

static midi_controller_t* persistent_controllers[] =
{
	&master_volume_controller,
	&waveform_controller,
	&oscilloscope_controller,
	&envelope_controller[0].attack_time_controller,
	&envelope_controller[0].attack_level_controller,
	&envelope_controller[0].decay_time_controller,
	&envelope_controller[0].decay_level_controller,
	&envelope_controller[0].sustain_time_controller,
	&envelope_controller[0].release_time_controller,
	&envelope_controller[1].attack_time_controller,
	&envelope_controller[1].attack_level_controller,
	&envelope_controller[1].decay_time_controller,
	&envelope_controller[1].decay_level_controller,
	&envelope_controller[1].sustain_time_controller,
	&envelope_controller[1].release_time_controller,
	&envelope_controller[2].attack_time_controller,
	&envelope_controller[2].attack_level_controller,
	&envelope_controller[2].decay_time_controller,
	&envelope_controller[2].decay_level_controller,
	&envelope_controller[2].sustain_time_controller,
	&envelope_controller[2].release_time_controller,
	&lfo_waveform_controller,
	&lfo_level_controller,
	&lfo_frequency_controller,
	&filter_state_controller,
	&filter_frequency_controller,
	&filter_q_controller,
};

typedef struct synth_controller_default_t {
	midi_controller_t*	controller;
	int					default_value;
} synth_controller_default_t;

// These mirror values defined in main.c - really this should be the source of default values.
// Ultimately defaults should be defined in a config file.
static synth_controller_default_t controller_defaults[] =
{
	{ &envelope_controller[0].attack_time_controller,	100 },
	{ &envelope_controller[0].attack_level_controller,	MOD_MATRIX_ONE },
	{ &envelope_controller[0].decay_time_controller,	250 },
	{ &envelope_controller[0].decay_level_controller,	MOD_MATRIX_ONE / 2 },
	{ &envelope_controller[0].sustain_time_controller,	DURATION_HELD },
	{ &envelope_controller[0].release_time_controller,	100 },

	{ &envelope_controller[1].attack_time_controller,	100 },
	{ &envelope_controller[1].attack_level_controller,	MOD_MATRIX_ONE },
	{ &envelope_controller[1].decay_time_controller,	250 },
	{ &envelope_controller[1].decay_level_controller,	MOD_MATRIX_ONE / 2},
	{ &envelope_controller[1].sustain_time_controller,	DURATION_HELD },
	{ &envelope_controller[1].release_time_controller,	100 },

	{ &envelope_controller[2].attack_time_controller,	100 },
	{ &envelope_controller[2].attack_level_controller,	MOD_MATRIX_ONE },
	{ &envelope_controller[2].decay_time_controller,	250 },
	{ &envelope_controller[2].decay_level_controller,	MOD_MATRIX_ONE / 2},
	{ &envelope_controller[2].sustain_time_controller,	DURATION_HELD },
	{ &envelope_controller[2].release_time_controller,	100 },
};

static void set_controller_defaults()
{
	for (int i = 0; i < sizeof(controller_defaults) / sizeof(controller_defaults[0]); i++)
	{
		controller_defaults[i].controller->last_output = controller_defaults[i].default_value;
	}
}

static void master_volume_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = 0;
	controller->output_max = LEVEL_MAX;
}

static void waveform_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = WAVE_FIRST_AUDIBLE;
	controller->output_max = WAVE_LAST_AUDIBLE;
}

static void midi_resolution_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = 0;
	controller->output_max = MIDI_MAX_CONTROLLER_VALUE;
}

static void envelope_level_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = 0;
	controller->output_max = MOD_MATRIX_ONE;
}

static void envelope_time_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = 0;
	controller->output_max = ENVELOPE_TIME_SCALE * MIDI_MAX_CONTROLLER_VALUE;
}

static void envelope_time_held_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = 0;
	controller->output_max = ENVELOPE_TIME_SCALE * MIDI_MAX_CONTROLLER_VALUE;
	controller->output_held = DURATION_HELD;
}

static void lfo_waveform_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = WAVE_FIRST_LFO;
	controller->output_max = WAVE_LAST_LFO;
}

static void lfo_frequency_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = LFO_MIN_FREQUENCY;
	controller->output_max = LFO_MAX_FREQUENCY;
}

static void level_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = 0;
	controller->output_max = LEVEL_MAX;
}

static void filter_state_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = FILTER_STATE_OFF;
	controller->output_max = FILTER_STATE_LAST;
}

static void filter_frequency_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = FILTER_MIN_FREQUENCY;
	controller->output_max = FILTER_MAX_FREQUENCY;
}

static void filter_q_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = FILTER_MIN_Q;
	controller->output_max = FILTER_MAX_Q;
}

typedef struct controller_parser_t
{
	const char *config_setting_name;
	midi_controller_t *controller;
	void(*set_output)(midi_controller_t *controller);
} controller_parser_t;

controller_parser_t controller_parser[] =
{
	{ "master_volume", &master_volume_controller, master_volume_controller_set_output },
	{ "waveform_select", &waveform_controller, waveform_controller_set_output },
	{ "oscilloscope_frequency", &oscilloscope_controller, midi_resolution_controller_set_output },
	{ "envelope_1_attack_time", &envelope_controller[0].attack_time_controller, envelope_time_controller_set_output },
	{ "envelope_1_attack_level", &envelope_controller[0].attack_level_controller, envelope_level_controller_set_output },
	{ "envelope_1_decay_time", &envelope_controller[0].decay_time_controller, envelope_time_controller_set_output },
	{ "envelope_1_decay_level", &envelope_controller[0].decay_level_controller, envelope_level_controller_set_output },
	{ "envelope_1_sustain_time", &envelope_controller[0].sustain_time_controller, envelope_time_held_controller_set_output },
	{ "envelope_1_release_time", &envelope_controller[0].release_time_controller, envelope_time_controller_set_output },
	{ "envelope_2_attack_time", &envelope_controller[1].attack_time_controller, envelope_time_controller_set_output },
	{ "envelope_2_attack_level", &envelope_controller[1].attack_level_controller, envelope_level_controller_set_output },
	{ "envelope_2_decay_time", &envelope_controller[1].decay_time_controller, envelope_time_controller_set_output },
	{ "envelope_2_decay_level", &envelope_controller[1].decay_level_controller, envelope_level_controller_set_output },
	{ "envelope_2_sustain_time", &envelope_controller[1].sustain_time_controller, envelope_time_held_controller_set_output },
	{ "envelope_2_release_time", &envelope_controller[1].release_time_controller, envelope_time_controller_set_output },
	{ "envelope_3_attack_time", &envelope_controller[2].attack_time_controller, envelope_time_controller_set_output },
	{ "envelope_3_attack_level", &envelope_controller[2].attack_level_controller, envelope_level_controller_set_output },
	{ "envelope_3_decay_time", &envelope_controller[2].decay_time_controller, envelope_time_controller_set_output },
	{ "envelope_3_decay_level", &envelope_controller[2].decay_level_controller, envelope_level_controller_set_output },
	{ "envelope_3_sustain_time", &envelope_controller[2].sustain_time_controller, envelope_time_held_controller_set_output },
	{ "envelope_3_release_time", &envelope_controller[2].release_time_controller, envelope_time_controller_set_output },
	{ "lfo_waveform_select", &lfo_waveform_controller, lfo_waveform_controller_set_output },
	{ "lfo_frequency", &lfo_frequency_controller, lfo_frequency_controller_set_output },
	{ "lfo_level", &lfo_level_controller, level_controller_set_output },
	{ "filter_state", &filter_state_controller, filter_state_controller_set_output },
	{ "filter_frequency", &filter_frequency_controller, filter_frequency_controller_set_output },
	{ "filter_q", &filter_q_controller, filter_q_controller_set_output },
	{ "exit", &exit_controller, NULL },
	{ "profile", &profile_controller, NULL },
	{ "screenshot", &screenshot_controller, NULL },
};

#define CONTROLLER_PARSER_COUNT	(sizeof(controller_parser) / sizeof(controller_parser[0]))

int synth_controllers_initialise(int controller_channel, config_setting_t *config)
{
	if (config == NULL)
	{
		LOG_ERROR("Missing controller configuration");
		return 0;
	}

	int error_count = 0;

	if (midi_controller_parse_index_controls(config, controller_channel) != RESULT_OK)
	{
		error_count++;
	}

	envelope_controller[0].renderer_id = ENVELOPE_1_RENDERER_ID;
	envelope_controller[1].renderer_id = ENVELOPE_2_RENDERER_ID;
	envelope_controller[2].renderer_id = ENVELOPE_3_RENDERER_ID;

	for (int i = 0; i < CONTROLLER_PARSER_COUNT; i++)
	{
		config_setting_t *setting_controller = config_setting_get_member(config, controller_parser[i].config_setting_name);
		if (setting_controller == NULL)
		{
			setting_error_report(config, "Controller config section %s missing", controller_parser[i].config_setting_name);
			error_count++;
		}
		else
		{
			midi_controller_create(controller_parser[i].controller, controller_parser[i].config_setting_name);
			controller_parser[i].controller->midi_channel = controller_channel;

			if (midi_controller_parse_config(setting_controller, controller_parser[i].controller) == RESULT_OK)
			{
				if (controller_parser[i].set_output != NULL)
				{
					controller_parser[i].set_output(controller_parser[i].controller);
				}
				midi_controller_init(controller_parser[i].controller);
			}
			else
			{
				error_count++;
			}
		}
	}

	set_controller_defaults();

	return error_count == 0;
}

void update_midi_controllers(synth_state_t* synth_state)
{
	midi_controller_update_index_controls();

	if (midi_controller_update(&master_volume_controller))
	{
		synth_state->volume = STATE_UPDATED;
	}

	if (midi_controller_update(&waveform_controller))
	{
		synth_state->waveform = STATE_UPDATED;
	}

	for (int i = 0; i < SYNTH_ENVELOPE_COUNT; i++)
	{
		if (midi_controller_update(&envelope_controller[i].attack_level_controller))
		{
			synth_state->envelope[i] = STATE_UPDATED;
		}

		if (midi_controller_update(&envelope_controller[i].attack_time_controller))
		{
			synth_state->envelope[i] = STATE_UPDATED;
		}

		if (midi_controller_update(&envelope_controller[i].decay_level_controller))
		{
			synth_state->envelope[i] = STATE_UPDATED;
		}

		if (midi_controller_update(&envelope_controller[i].decay_time_controller))
		{
			synth_state->envelope[i] = STATE_UPDATED;
		}

		if (midi_controller_update(&envelope_controller[i].sustain_time_controller))
		{
			synth_state->envelope[i] = STATE_UPDATED;
		}

		if (midi_controller_update(&envelope_controller[i].release_time_controller))
		{
			synth_state->envelope[i] = STATE_UPDATED;
		}
	}

	if (midi_controller_update(&lfo_waveform_controller))
	{
		synth_state->lfo_params = STATE_UPDATED;
	}

	if (midi_controller_update(&lfo_level_controller))
	{
		synth_state->lfo_params = STATE_UPDATED;
	}

	if (midi_controller_update(&lfo_frequency_controller))
	{
		synth_state->lfo_params = STATE_UPDATED;
	}

	if (midi_controller_update(&filter_state_controller))
	{
		synth_state->filter = STATE_UPDATED;
	}

	if (midi_controller_update(&filter_frequency_controller))
	{
		synth_state->filter = STATE_UPDATED;
	}

	if (midi_controller_update(&filter_q_controller))
	{
		synth_state->filter = STATE_UPDATED;
	}
}

void update_envelope(envelope_t* envelope, object_id_t envelope_renderer_id, envelope_controller_t* envelope_controller)
{
	envelope->stages[ENVELOPE_STAGE_ATTACK].end_level 		= midi_controller_read(&envelope_controller->attack_level_controller);
	envelope->stages[ENVELOPE_STAGE_DECAY].start_level 		= midi_controller_read(&envelope_controller->attack_level_controller);
	envelope->stages[ENVELOPE_STAGE_ATTACK].duration 		= midi_controller_read(&envelope_controller->attack_time_controller);
	envelope->stages[ENVELOPE_STAGE_DECAY].end_level 		= midi_controller_read(&envelope_controller->decay_level_controller);
	envelope->stages[ENVELOPE_STAGE_SUSTAIN].start_level 	= midi_controller_read(&envelope_controller->decay_level_controller);
	envelope->stages[ENVELOPE_STAGE_SUSTAIN].end_level 		= midi_controller_read(&envelope_controller->decay_level_controller);
	envelope->stages[ENVELOPE_STAGE_DECAY].duration 		= midi_controller_read(&envelope_controller->decay_time_controller);
	envelope->stages[ENVELOPE_STAGE_SUSTAIN].duration 		= midi_controller_read(&envelope_controller->sustain_time_controller);
	envelope->stages[ENVELOPE_STAGE_RELEASE].duration 		= midi_controller_read(&envelope_controller->release_time_controller);

	gfx_event_t gfx_event;
	gfx_event.type = GFX_EVENT_REFRESH;
	gfx_event.flags = 0;
	gfx_event.receiver_id = envelope_renderer_id;
	gfx_send_event(&gfx_event);
}

void update_synth(synth_state_t* synth_state, synth_model_t* synth_model)
{
	if (synth_state->volume == STATE_UPDATED)
	{
		midi_controller_set_setting(&master_volume_controller, synth_model->setting_master_volume);
		gfx_event_t gfx_event;
		gfx_event.type = GFX_EVENT_REFRESH;
		gfx_event.flags = 0;
		gfx_event.receiver_id = MASTER_VOLUME_RENDERER_ID;
		gfx_send_event(&gfx_event);
	}

	if (synth_state->waveform == STATE_UPDATED)
	{
		midi_controller_set_setting(&waveform_controller, synth_model->setting_master_waveform);
		gfx_event_t gfx_event;
		gfx_event.type = GFX_EVENT_REFRESH;
		gfx_event.flags = 0;
		gfx_event.receiver_id = MASTER_WAVEFORM_RENDERER_ID;
		gfx_send_event(&gfx_event);
	}

	for (int i = 0; i < SYNTH_ENVELOPE_COUNT; i++)
	{
		if (synth_state->envelope[i] == STATE_UPDATED)
		{
			update_envelope(&synth_model->envelope[i], envelope_controller[i].renderer_id, &envelope_controller[i]);
		}
	}

	if (synth_state->lfo_params == STATE_UPDATED)
	{
		synth_model->lfo_def.oscillator.waveform = midi_controller_read(&lfo_waveform_controller);
		synth_model->lfo_def.oscillator.level = midi_controller_read(&lfo_level_controller);
		synth_model->lfo_def.oscillator.frequency = midi_controller_read(&lfo_frequency_controller);
	}

	if (synth_state->filter == STATE_UPDATED)
	{
		synth_model->global_filter_def.type = midi_controller_read(&filter_state_controller);
		synth_model->global_filter_def.frequency = midi_controller_read(&filter_frequency_controller);
		synth_model->global_filter_def.q = midi_controller_read(&filter_q_controller);
	}
}

void process_synth_controllers(synth_model_t* synth_model)
{
	synth_state_t synth_state;
	memset(&synth_state, 0, sizeof(synth_state));
	update_midi_controllers(&synth_state);
	update_synth(&synth_state, synth_model);
}

int synth_controllers_save(const char* file_path)
{
	FILE* file = fopen(file_path, "wb");
	if (file != NULL)
	{
		size_t controller_count = sizeof(persistent_controllers) / sizeof(persistent_controllers[0]);

		if (fwrite(&controller_count, sizeof(size_t), 1, file) != 1)
		{
			push_last_error();
			fclose(file);
			goto error;
		}

		for (size_t i = 0; i < controller_count; i++)
		{
			if (fwrite(&(persistent_controllers[i]->last_output), sizeof(size_t), 1, file) != 1)
			{
				push_last_error();
				fclose(file);
				goto error;
			}
		}

		if (fclose(file) != 0)
		{
			push_last_error();
			goto error;
		}
	}
	else
	{
		push_last_error();
		goto error;
	}

	return RESULT_OK;

error:
	pop_error_report("synth_controllers_save failed");
	return RESULT_ERROR;
}

int synth_controllers_load(const char* file_path, synth_model_t* synth_model)
{
	FILE* file = fopen(file_path, "rb");
	if (file != NULL)
	{
		size_t controller_count = sizeof(persistent_controllers) / sizeof(persistent_controllers[0]);
		size_t loaded_controller_count;
		if (fread(&loaded_controller_count, sizeof(size_t), 1, file) != 1)
		{
			push_last_error();
			fclose(file);
			goto error;
		}

		if (loaded_controller_count == controller_count)
		{
			for (size_t i = 0; i < controller_count; i++)
			{
				if (fread(&(persistent_controllers[i]->last_output), sizeof(size_t), 1, file) != 1)
				{
					push_last_error();
					fclose(file);
					goto error;
				}
			}
		}
		else
		{
			push_custom_error("saved controller count mismatch");
			goto error;
		}

		if (fclose(file) != 0)
		{
			push_last_error();
			goto error;
		}
	}
	else
	{
		push_last_error();
		goto error;
	}

	synth_state_t synth_state;
	memset(&synth_state, STATE_UPDATED, sizeof(synth_state));
	update_synth(&synth_state, synth_model);

	return RESULT_OK;

error:
	pop_error_report("synth_controllers_load failed");
	return RESULT_ERROR;
}
