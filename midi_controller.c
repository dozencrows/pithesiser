/*
 * midi_controller.c
 *
 *  Created on: 11 Feb 2013
 *      Author: ntuckett
 */

#include "midi_controller.h"
#include <memory.h>
#include "midi.h"
#include "setting.h"

static int read_midi_controller(midi_controller_t* controller, int* value)
{
	int changed = 0;
	if (controller->midi_cc[0] != -1)
	{
		if (controller->midi_cc[1] != -1)
		{
			int msb, lsb;

			changed = midi_get_raw_controller_changed(controller->midi_channel, controller->midi_cc[0]);
			msb = midi_get_raw_controller_value(controller->midi_channel, controller->midi_cc[0]);
			changed |= midi_get_raw_controller_changed(controller->midi_channel, controller->midi_cc[1]);
			lsb = midi_get_raw_controller_value(controller->midi_channel, controller->midi_cc[1]);
			*value = lsb | msb << 7;
		}
		else if (midi_get_raw_controller_changed(controller->midi_channel, controller->midi_cc[0]))
		{
			*value = midi_get_raw_controller_value(controller->midi_channel, controller->midi_cc[0]);
			changed = 1;
		}
	}

	return changed;
}

static int process_continuous_controller(midi_controller_t* controller, int* changed)
{
	int midi_value;

	if (read_midi_controller(controller, &midi_value))
	{
		*changed = 1;
		int midi_range = controller->midi_range.max - controller->midi_range.min;

		if (midi_value < controller->midi_range.min)
		{
			midi_value = 0;
		}
		else if (midi_value > controller->midi_range.max)
		{
			midi_value = midi_range;
		}
		else
		{
			midi_value -= controller->midi_range.min;
		}

		int64_t controller_range = controller->output_max - controller->output_min;
		controller->last_output = controller->output_min + ((controller_range * (int64_t)midi_value) / (int64_t)midi_range);
	}

	return controller->last_output;
}

static int process_continuous_controller_with_end(midi_controller_t* controller, int* changed)
{
	int midi_value;

	if (read_midi_controller(controller, &midi_value))
	{
		*changed = 1;
		int midi_range = controller->midi_range.max - controller->midi_range.min;

		if (midi_value < controller->midi_range.min)
		{
			midi_value = controller->midi_range.min;
		}

		if (midi_value > controller->midi_range.max)
		{
			controller->last_output = controller->output_held;
		}
		else
		{
			midi_value -= controller->midi_range.min;
			int controller_range = controller->output_max - controller->output_min;
			controller->last_output = controller->output_min + ((controller_range * midi_value) / midi_range);
		}
	}

	return controller->last_output;
}

static int process_toggle_controller(midi_controller_t* controller, int* changed)
{
	int midi_value;

	if (read_midi_controller(controller, &midi_value))
	{
		*changed = 1;
		if (midi_value > controller->midi_threshold)
		{
			controller->last_output++;
			if (controller->last_output > controller->output_max)
			{
				controller->last_output = controller->output_min;
			}
		}
	}

	return controller->last_output;
}

static int process_event_controller(midi_controller_t* controller, int* changed)
{
	int midi_value;

	if (read_midi_controller(controller, &midi_value))
	{
		if (midi_value > controller->midi_threshold && controller->last_output < controller->midi_threshold)
		{
			*changed = 1;
		}

		controller->last_output = midi_value;
	}

	if (*changed)
	{
		return controller->last_output;
	}
	else
	{
		return 0;
	}
}

void midi_controller_create(midi_controller_t* controller)
{
	memset(controller, 0, sizeof(midi_controller_t));
	controller->midi_cc[0] = -1;
	controller->midi_cc[1] = -1;
	controller->type = NONE;
}

void midi_controller_init(midi_controller_t* controller)
{
	controller->last_output = controller->output_min;
}

int midi_controller_update_and_read(midi_controller_t* controller, int* value)
{
	int changed = 0;

	switch (controller->type)
	{
		case CONTINUOUS:
		{
			*value = process_continuous_controller(controller, &changed);
			break;
		}

		case CONTINUOUS_WITH_HELD:
		{
			*value = process_continuous_controller_with_end(controller, &changed);
			break;
		}

		case TOGGLE:
		{
			*value = process_toggle_controller(controller, &changed);
			break;
		}

		case EVENT:
		{
			*value = process_event_controller(controller, &changed);
			break;
		}

		default:
		{
			break;
		}
	}

	return changed;
}

int midi_controller_update(midi_controller_t* controller)
{
	int dummy_value;
	return midi_controller_update_and_read(controller, &dummy_value);
}

int midi_controller_read(midi_controller_t* controller)
{
	return controller->last_output;
}

void midi_controller_set_setting(midi_controller_t* controller, setting_t* setting)
{
	int value = midi_controller_read(controller);
	setting_set_value_int(setting, value);
}
