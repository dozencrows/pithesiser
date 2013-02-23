/*
 * synth_controllers.c
 *
 *  Created on: 11 Feb 2013
 *      Author: ntuckett
 */

#include "synth_controllers.h"
#include <stdarg.h>
#include <string.h>
#include "system_constants.h"
#include "midi.h"
#include "envelope.h"
#include "waveform.h"
#include "filter.h"

#define LFO_MIN_FREQUENCY		(FIXED_ONE / 10)
#define LFO_MAX_FREQUENCY		(20 * FIXED_ONE)
#define LFO_STATE_LAST			LFO_STATE_PITCH

#define FILTER_STATE_OFF		FILTER_PASS
#define FILTER_STATE_LAST		FILTER_HPF
#define FILTER_MIN_FREQUENCY	(FILTER_FIXED_ONE * 20)
#define FILTER_MAX_FREQUENCY	(18000 * FILTER_FIXED_ONE)
#define FILTER_MIN_Q			(FIXED_ONE / 100)
#define FILTER_MAX_Q			(FIXED_ONE)
#define ENVELOPE_TIME_SCALE				10

midi_controller_t master_volume_controller;
midi_controller_t waveform_controller;
midi_controller_t oscilloscope_controller;

midi_controller_t envelope_attack_time_controller;
midi_controller_t envelope_attack_level_controller;
midi_controller_t envelope_decay_time_controller;
midi_controller_t envelope_decay_level_controller;
midi_controller_t envelope_sustain_time_controller;
midi_controller_t envelope_release_time_controller;

midi_controller_t lfo_state_controller;
midi_controller_t lfo_waveform_controller;
midi_controller_t lfo_level_controller;
midi_controller_t lfo_frequency_controller;

midi_controller_t filter_state_controller;
midi_controller_t filter_frequency_controller;
midi_controller_t filter_q_controller;

midi_controller_t exit_controller;
midi_controller_t profile_controller;

static const char* CFG_TYPE_SETTING = "type";
static const char* CFG_CONTINUOUS_CONTROLLER = "continuous";
static const char* CFG_CONTINUOUS_WITH_HELD_CONTROLLER = "continuous_with_held";
static const char* CFG_TOGGLE_CONTROLLER = "toggle";
static const char* CFG_EVENT_CONTROLLER = "event";
static const char* CFG_MIN = "min";
static const char* CFG_MAX = "max";
static const char* CFG_THRESHOLD = "threshold";

static void report_config_error(config_setting_t *setting, const char* message, ...)
{
	va_list arg_list;
	va_start(arg_list, message);
	vprintf(message, arg_list);
	va_end(arg_list);
	printf(" at line %d in %s\n", config_setting_source_line(setting), config_setting_source_file(setting));
}

static int parse_midi_cc(config_setting_t *config, midi_controller_t *controller)
{
	config_setting_t *setting_midi_cc = config_setting_get_member(config, "midi_cc");

	if (setting_midi_cc == NULL)
	{
		report_config_error(config, "Missing midi_cc data");
		return 0;
	}

	int midi_cc_count = config_setting_length(setting_midi_cc);

	if (midi_cc_count < 1 || midi_cc_count > 2)
	{
		report_config_error(config, "Only 1 or 2 midi_cc allowed - got %d", midi_cc_count);
		return 0;
	}

	for (int i = 0; i < midi_cc_count; i++)
	{
		controller->midi_cc[i] = config_setting_get_int_elem(setting_midi_cc, i);
	}

	return 1;
}

static int parse_midi_max_min(config_setting_t *config, midi_controller_t *controller)
{
	if (config_setting_lookup_int(config, CFG_MIN, &(controller->midi_range.min)) != CONFIG_TRUE)
	{
		report_config_error(config, "Missing or invalid min");
		return 0;
	}

	if (config_setting_lookup_int(config, CFG_MAX, &(controller->midi_range.max)) != CONFIG_TRUE)
	{
		report_config_error(config, "Missing or invalid max");
		return 0;
	}

	return 1;
}

static int parse_midi_threshold(config_setting_t *config, midi_controller_t *controller)
{
	if (config_setting_lookup_int(config, CFG_THRESHOLD, &(controller->midi_threshold)) != CONFIG_TRUE)
	{
		report_config_error(config, "Missing or invalid threshold");
		return 0;
	}

	return 1;
}

static int parse_continuous_controller(config_setting_t *config, midi_controller_t *controller)
{
	controller->type = CONTINUOUS;
	if (!parse_midi_cc(config, controller))
	{
		return 0;
	}

	return parse_midi_max_min(config, controller);
}

static int parse_continuous_with_end_controller(config_setting_t *config, midi_controller_t *controller)
{
	controller->type = CONTINUOUS_WITH_HELD;
	if (!parse_midi_cc(config, controller))
	{
		return 0;
	}

	return parse_midi_max_min(config, controller);
}

static int parse_toggle_controller(config_setting_t *config, midi_controller_t *controller)
{
	controller->type = TOGGLE;
	if (!parse_midi_cc(config, controller))
	{
		return 0;
	}

	return parse_midi_threshold(config, controller);
}

static int parse_event_controller(config_setting_t *config, midi_controller_t *controller)
{
	controller->type = EVENT;
	if (!parse_midi_cc(config, controller))
	{
		return 0;
	}

	return parse_midi_threshold(config, controller);
}


static int parse_controller_config(config_setting_t *config, midi_controller_t *controller)
{
	const char* type_setting;

	if (config_setting_lookup_string(config, CFG_TYPE_SETTING, &type_setting) != CONFIG_TRUE)
	{
		report_config_error(config, "Controller needs type");
	}

	if (strcasecmp(type_setting, CFG_CONTINUOUS_CONTROLLER) == 0)
	{
		return parse_continuous_controller(config, controller);
	}
	else if (strcasecmp(type_setting, CFG_CONTINUOUS_WITH_HELD_CONTROLLER) == 0)
	{
		return parse_continuous_with_end_controller(config, controller);
	}
	else if (strcasecmp(type_setting, CFG_TOGGLE_CONTROLLER) == 0)
	{
		return parse_toggle_controller(config, controller);
	}
	else if (strcasecmp(type_setting, CFG_EVENT_CONTROLLER) == 0)
	{
		return parse_event_controller(config, controller);
	}
	else
	{
		report_config_error(config, "Unsupported controller type %s", type_setting);
	}

	return 0;
}

typedef struct controller_parser_t
{
	const char *config_setting_name;
	midi_controller_t *controller;
	void(*set_output)(midi_controller_t *controller);
} controller_parser_t;

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
	controller->output_max = ENVELOPE_LEVEL_MAX;
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

static void lfo_state_controller_set_output(midi_controller_t *controller)
{
	controller->output_min = LFO_STATE_OFF;
	controller->output_max = LFO_STATE_LAST;
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

controller_parser_t controller_parser[] =
{
	{ "master_volume", &master_volume_controller, master_volume_controller_set_output },
	{ "waveform_select", &waveform_controller, waveform_controller_set_output },
	{ "oscilloscope_frequency", &oscilloscope_controller, midi_resolution_controller_set_output },
	{ "volume_envelope_attack_time", &envelope_attack_time_controller, envelope_time_controller_set_output },
	{ "volume_envelope_attack_level", &envelope_attack_level_controller, envelope_level_controller_set_output },
	{ "volume_envelope_decay_time", &envelope_decay_time_controller, envelope_time_controller_set_output },
	{ "volume_envelope_decay_level", &envelope_decay_level_controller, envelope_level_controller_set_output },
	{ "volume_envelope_sustain_time", &envelope_sustain_time_controller, envelope_time_held_controller_set_output },
	{ "volume_envelope_release_time", &envelope_release_time_controller, envelope_time_controller_set_output },
	{ "lfo_state", &lfo_state_controller, lfo_state_controller_set_output },
	{ "lfo_waveform_select", &lfo_waveform_controller, lfo_waveform_controller_set_output },
	{ "lfo_frequency", &lfo_frequency_controller, lfo_frequency_controller_set_output },
	{ "lfo_level", &lfo_level_controller, level_controller_set_output },
	{ "filter_state", &filter_state_controller, filter_state_controller_set_output },
	{ "filter_frequency", &filter_frequency_controller, filter_frequency_controller_set_output },
	{ "filter_q", &filter_q_controller, filter_q_controller_set_output },
	{ "exit", &exit_controller, NULL },
	{ "profile", &profile_controller, NULL },
};

#define CONTROLLER_PARSER_COUNT	(sizeof(controller_parser) / sizeof(controller_parser[0]))

int synth_controllers_initialise(int controller_channel, config_setting_t *config)
{
	if (config == NULL)
	{
		printf("Missing controller configuration\n");
		return 0;
	}

	int error_count = 0;

	for (int i = 0; i < CONTROLLER_PARSER_COUNT; i++)
	{
		config_setting_t *setting_controller = config_setting_get_member(config, controller_parser[i].config_setting_name);
		if (setting_controller == NULL)
		{
			report_config_error(config, "Controller config section %s missing", controller_parser[i].config_setting_name);
			error_count++;
		}
		else
		{
			midi_controller_create(controller_parser[i].controller);
			controller_parser[i].controller->midi_channel = controller_channel;

			if (parse_controller_config(setting_controller, controller_parser[i].controller))
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

	return error_count == 0;
}
