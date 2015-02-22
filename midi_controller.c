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

static int relative_controller_delta(int midi_delta, midi_controller_t* controller)
{
	int controller_delta;

	if (controller->midi_cc[1] != -1)
	{
		const int SIGN_EXTEND_14BITS_M = 1U << 13;
		controller_delta = (midi_delta ^ SIGN_EXTEND_14BITS_M) - SIGN_EXTEND_14BITS_M;
	}
	else
	{
		const int SIGN_EXTEND_7BITS_M = 1U << 6;
		controller_delta = (midi_delta ^ SIGN_EXTEND_7BITS_M) - SIGN_EXTEND_7BITS_M;
	}

	return controller_delta * controller->delta_scale;
}

static int process_continuous_relative_controller(midi_controller_t* controller, int* changed)
{
	int midi_delta;

	if (read_midi_controller(controller, &midi_delta))
	{
		if (midi_delta != 0)
		{
			*changed = 1;

			controller->last_output += relative_controller_delta(midi_delta, controller);

			if (controller->last_output < controller->output_min)
			{
				controller->last_output = controller->output_min;
			}
			else if (controller->last_output > controller->output_max)
			{
				controller->last_output = controller->output_max;
			}
		}
	}

	return controller->last_output;
}

static int process_continuous_relative_controller_with_held(midi_controller_t* controller, int* changed)
{
	int midi_delta;

	if (read_midi_controller(controller, &midi_delta))
	{
		if (midi_delta != 0)
		{
			*changed = 1;

			if (controller->last_output == controller->output_held)
			{
				controller->last_output = controller->output_max;
			}

			controller->last_output += relative_controller_delta(midi_delta, controller);

			if (controller->last_output < controller->output_min)
			{
				controller->last_output = controller->output_min;
			}
			else if (controller->last_output > controller->output_max)
			{
				controller->last_output = controller->output_held;
			}
		}
	}

	return controller->last_output;
}

static int process_continuous_controller_with_held(midi_controller_t* controller, int* changed)
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

void midi_controller_create(midi_controller_t* controller, const char* name)
{
	memset(controller, 0, sizeof(midi_controller_t));
	controller->name = name;
	controller->midi_cc[0] = -1;
	controller->midi_cc[1] = -1;
	controller->type = NONE;
	controller->delta_scale	= 1;
}

void midi_controller_init(midi_controller_t* controller)
{
	controller->last_output = controller->output_min;
}

int midi_controller_update_and_read(midi_controller_t* controller, int* value)
{
	int changed = 0;

	if (controller->indexer_control != NULL)
	{
		if (midi_controller_read(controller->indexer_control) != controller->indexer_value)
		{
			return changed;
		}
	}

	switch (controller->type)
	{
		case CONTINUOUS:
		{
			*value = process_continuous_controller(controller, &changed);
			break;
		}

		case CONTINUOUS_RELATIVE:
		{
			*value = process_continuous_relative_controller(controller, &changed);
			break;
		}

		case CONTINUOUS_WITH_HELD:
		{
			*value = process_continuous_controller_with_held(controller, &changed);
			break;
		}

		case CONTINUOUS_RELATIVE_WITH_HELD:
		{
			*value = process_continuous_relative_controller_with_held(controller, &changed);
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

#define MAX_INDEX_CONTROLS	16
static midi_controller_t index_controls[MAX_INDEX_CONTROLS];
static int next_free_index_control = 0;

midi_controller_t* midi_controller_new_index_control(const char* name)
{
	if (next_free_index_control < MAX_INDEX_CONTROLS)
	{
		midi_controller_t* index_control = index_controls + next_free_index_control;
		next_free_index_control++;
		midi_controller_create(index_control, name);
		return index_control;
	}
	else
	{
		return NULL;
	}
}

midi_controller_t* midi_controller_find_index_control(const char* name)
{
	for (int i = 0; i < next_free_index_control; i++)
	{
		if (strcasecmp(name, index_controls[i].name) == 0)
		{
			return index_controls + i;
		}
	}

	return NULL;
}

void midi_controller_update_index_controls()
{
	for (int i = 0; i < next_free_index_control; i++)
	{
		midi_controller_update(index_controls + i);
	}
}
