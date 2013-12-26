/*
 * midi_controller_parser.c
 *
 *  Created on: 26 Dec 2013
 *      Author: ntuckett
 */

#include "midi_controller_parser.h"
#include <stdlib.h>
#include <stdarg.h>
#include "libconfig.h"
#include "midi_controller.h"

static const char* CFG_TYPE_SETTING = "type";
static const char* CFG_CONTINUOUS_CONTROLLER = "continuous";
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

static int parse_midi_cc(config_setting_t *config, midi_controller_t *controller)
{
	config_setting_t *setting_midi_cc = config_setting_get_member(config, "midi_cc");

	if (setting_midi_cc == NULL)
	{
		midi_controller_parser_report_error(config, "Missing midi_cc data");
		return 0;
	}

	int midi_cc_count = config_setting_length(setting_midi_cc);

	if (midi_cc_count < 1 || midi_cc_count > 2)
	{
		midi_controller_parser_report_error(config, "Only 1 or 2 midi_cc allowed - got %d", midi_cc_count);
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
		midi_controller_parser_report_error(config, "Missing or invalid min");
		return 0;
	}

	if (config_setting_lookup_int(config, CFG_MAX, &(controller->midi_range.max)) != CONFIG_TRUE)
	{
		midi_controller_parser_report_error(config, "Missing or invalid max");
		return 0;
	}

	return 1;
}

static int parse_midi_threshold(config_setting_t *config, midi_controller_t *controller)
{
	if (config_setting_lookup_int(config, CFG_THRESHOLD, &(controller->midi_threshold)) != CONFIG_TRUE)
	{
		midi_controller_parser_report_error(config, "Missing or invalid threshold");
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

	return 0;
}

