/*
 * midi_controller_parser.c
 *
 *  Created on: 26 Dec 2013
 *      Author: ntuckett
 */

#include "midi_controller_parser.h"
#include "system_constants.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libconfig.h"
#include "midi_controller.h"

static const char* CFG_TYPE_SETTING = "type";
static const char* CFG_CONTINUOUS_CONTROLLER = "continuous";
static const char* CFG_CONTINUOUS_RELATIVE_CONTROLLER = "continuous_relative";
static const char* CFG_CONTINUOUS_WITH_HELD_CONTROLLER = "continuous_with_held";
static const char* CFG_TOGGLE_CONTROLLER = "toggle";
static const char* CFG_EVENT_CONTROLLER = "event";
static const char* CFG_MIN = "min";
static const char* CFG_MAX = "max";
static const char* CFG_THRESHOLD = "threshold";

void midi_controller_parser_report_error(config_setting_t *setting, const char* message, ...)
{
	va_list arg_list;
	va_start(arg_list, message);
	vprintf(message, arg_list);
	va_end(arg_list);
	printf(" at line %d in %s\n", config_setting_source_line(setting), config_setting_source_file(setting));
}

static int parse_indexer(config_setting_t *config, midi_controller_t *controller)
{
	const char* index_controller;
	int index_value;

	if (config_setting_lookup_string(config, "controller", &index_controller) != CONFIG_TRUE)
	{
		midi_controller_parser_report_error(config, "Missing controller for indexer");
		return RESULT_ERROR;
	}

	midi_controller_t* indexer = midi_controller_find_index_control(index_controller);

	if (indexer == NULL)
	{
		midi_controller_parser_report_error(config, "Unknown controller %s for indexer", index_controller);
		return RESULT_ERROR;
	}

	if (config_setting_lookup_int(config, "value", &index_value) != CONFIG_TRUE)
	{
		midi_controller_parser_report_error(config, "Missing value for indexer");
		return RESULT_ERROR;
	}

	if (index_value < indexer->output_min || index_value > indexer->output_max)
	{
		midi_controller_parser_report_error(config, "Value %d out of range for indexer (%d to %d)", index_value, indexer->output_min, indexer->output_max);
		return RESULT_ERROR;
	}

	controller->indexer_control = indexer;
	controller->indexer_value = index_value;
	return RESULT_OK;
}

static int parse_midi_cc(config_setting_t *config, midi_controller_t *controller)
{
	config_setting_t *setting_midi_cc = config_setting_get_member(config, "midi_cc");

	if (setting_midi_cc == NULL)
	{
		midi_controller_parser_report_error(config, "Missing midi_cc data");
		return RESULT_ERROR;
	}

	int midi_cc_count = config_setting_length(setting_midi_cc);

	if (midi_cc_count < 1 || midi_cc_count > 2)
	{
		midi_controller_parser_report_error(config, "Only 1 or 2 midi_cc allowed - got %d", midi_cc_count);
		return RESULT_ERROR;
	}

	for (int i = 0; i < midi_cc_count; i++)
	{
		controller->midi_cc[i] = config_setting_get_int_elem(setting_midi_cc, i);
	}

	config_setting_t *indexer = config_setting_get_member(config, "indexer");
	if (indexer != NULL)
	{
		return parse_indexer(indexer, controller);
	}
	else
	{
		return RESULT_OK;
	}
}

static int parse_midi_max_min(config_setting_t *config, midi_controller_t *controller)
{
	if (config_setting_lookup_int(config, CFG_MIN, &(controller->midi_range.min)) != CONFIG_TRUE)
	{
		midi_controller_parser_report_error(config, "Missing or invalid min");
		return RESULT_ERROR;
	}

	if (config_setting_lookup_int(config, CFG_MAX, &(controller->midi_range.max)) != CONFIG_TRUE)
	{
		midi_controller_parser_report_error(config, "Missing or invalid max");
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

static int parse_midi_threshold(config_setting_t *config, midi_controller_t *controller)
{
	if (config_setting_lookup_int(config, CFG_THRESHOLD, &(controller->midi_threshold)) != CONFIG_TRUE)
	{
		midi_controller_parser_report_error(config, "Missing or invalid threshold");
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

static int parse_continuous_controller(config_setting_t *config, midi_controller_t *controller)
{
	controller->type = CONTINUOUS;
	if (parse_midi_cc(config, controller) != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	return parse_midi_max_min(config, controller);
}

static int parse_continuous_relative_controller(config_setting_t *config, midi_controller_t *controller)
{
	controller->type = CONTINUOUS_RELATIVE;
	return parse_midi_cc(config, controller);
}

static int parse_continuous_with_end_controller(config_setting_t *config, midi_controller_t *controller)
{
	controller->type = CONTINUOUS_WITH_HELD;
	if (parse_midi_cc(config, controller) != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	return parse_midi_max_min(config, controller);
}

static int parse_toggle_controller(config_setting_t *config, midi_controller_t *controller)
{
	controller->type = TOGGLE;
	if (parse_midi_cc(config, controller) != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	return parse_midi_threshold(config, controller);
}

static int parse_event_controller(config_setting_t *config, midi_controller_t *controller)
{
	controller->type = EVENT;
	if (parse_midi_cc(config, controller) != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	return parse_midi_threshold(config, controller);
}

int midi_controller_parse_config(config_setting_t *config, midi_controller_t *controller)
{
	const char* type_setting;

	if (config_setting_lookup_string(config, CFG_TYPE_SETTING, &type_setting) != CONFIG_TRUE)
	{
		midi_controller_parser_report_error(config, "Controller needs type");
	}

	if (strcasecmp(type_setting, CFG_CONTINUOUS_CONTROLLER) == 0)
	{
		return parse_continuous_controller(config, controller);
	}
	else if (strcasecmp(type_setting, CFG_CONTINUOUS_RELATIVE_CONTROLLER) == 0)
	{
		return parse_continuous_relative_controller(config, controller);
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
		midi_controller_parser_report_error(config, "Unsupported controller type %s", type_setting);
	}

	return RESULT_ERROR;
}

static int add_index_control(config_setting_t* index_control_setting, int channel)
{
	const char* control_name = config_setting_name(index_control_setting);
	midi_controller_t* index_control = midi_controller_find_index_control(control_name);
	if (index_control != NULL)
	{
		midi_controller_parser_report_error(index_control_setting, "control %s already exists", control_name);
		return RESULT_ERROR;
	}

	index_control = midi_controller_new_index_control(control_name);
	if (index_control != NULL)
	{
		int result = midi_controller_parse_config(index_control_setting, index_control);
		if (result == RESULT_OK)
		{
			index_control->midi_channel = channel;
			index_control->output_min = index_control->midi_range.min;
			index_control->output_max = index_control->midi_range.max;
			midi_controller_init(index_control);
		}

		return result;
	}
	else
	{
		midi_controller_parser_report_error(index_control_setting, "index control limit");
		return RESULT_ERROR;
	}
}

int midi_controller_parse_index_controls(config_setting_t *config, int channel)
{
	config_setting_t* index_controls_setting = config_setting_get_member(config, "index_controls");

	if (index_controls_setting == NULL)
	{
		return RESULT_OK;
	}

	int error_count = 0;

	for(int i = 0; i < config_setting_length(index_controls_setting); i++)
	{
		config_setting_t* index_control_setting = config_setting_get_elem(index_controls_setting, i);

		if (index_control_setting != NULL)
		{
			if (add_index_control(index_control_setting, channel) != RESULT_OK)
			{
				error_count++;
			}
		}
		else
		{
			midi_controller_parser_report_error(index_controls_setting, "Could not find index control setting");
		}
	}

	return error_count == 0 ? RESULT_OK : RESULT_ERROR;
}
