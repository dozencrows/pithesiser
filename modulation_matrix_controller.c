/*
 * modulation_matrix_controller.c
 *
 *  Created on: 31 Dec 2013
 *      Author: ntuckett
 */

#include "modulation_matrix_controller.h"
#include <libconfig.h>
#include <syslog.h>
#include "system_constants.h"
#include "modulation_matrix.h"
#include "error_handler.h"
#include "midi.h"

#define MOD_MATRIX_CONTROL_NOTE_BASE		0
#define MOD_MATRIX_CONTROL_NOTE_STRIDE		16

static const char*	column_names[MOD_MATRIX_MAX_SOURCES];
static const char*	row_names[MOD_MATRIX_MAX_SINKS];
static int column_count = 0;
static int row_count = 0;
static int midi_channel = 0;

static const char* CFG_MOD_MATRIX_CHANNEL 	= "channel";
static const char* CFG_MOD_MATRIX_COLUMNS 	= "columns";
static const char* CFG_MOD_MATRIX_ROWS 		= "rows";

int parse_matrix_controller_settings(config_setting_t *config)
{
	int result = RESULT_OK;

	if (config == NULL)
	{
		return result;
	}

	if (config_setting_lookup_int(config, CFG_MOD_MATRIX_CHANNEL, &midi_channel) != CONFIG_TRUE)
	{
		setting_error_report(config, "Missing or invalid MIDI channel for modulation matrix");
		result = RESULT_ERROR;
	}

	config_setting_t* row_settings = config_setting_get_member(config, CFG_MOD_MATRIX_ROWS);
	if (row_settings == NULL)
	{
		setting_error_report(config, "Missing or invalid row list for modulation matrix");
		result = RESULT_ERROR;
	}
	else
	{
		row_count = config_setting_length(row_settings);

		if (row_count >= MOD_MATRIX_MAX_SINKS)
		{
			setting_error_report(row_settings, "Too many rows (%d), limit is %d", row_count, MOD_MATRIX_MAX_SINKS);
			result = RESULT_ERROR;
		}
		else
		{
			for (int i = 0; i < row_count; i++)
			{
				row_names[i] = config_setting_get_string_elem(row_settings, i);
				if (row_names[i] == NULL)
				{
					setting_error_report(row_settings, "Missing or invalid row name %d", i);
					result = RESULT_ERROR;
				}
			}
		}
	}

	config_setting_t* column_settings = config_setting_get_member(config, CFG_MOD_MATRIX_COLUMNS);
	if (column_settings == NULL)
	{
		setting_error_report(config, "Missing or invalid column list for modulation matrix");
		result = RESULT_ERROR;
	}
	else
	{
		column_count = config_setting_length(column_settings);

		if (column_count >= MOD_MATRIX_MAX_SOURCES)
		{
			setting_error_report(row_settings, "Too many columns (%d), limit is %d", column_count, MOD_MATRIX_MAX_SOURCES);
			result = RESULT_ERROR;
		}
		else
		{
			for (int i = 0; i < column_count; i++)
			{
				column_names[i] = config_setting_get_string_elem(column_settings, i);
				if (column_names[i] == NULL)
				{
					setting_error_report(column_settings, "Missing or invalid column name %d", i);
					result = RESULT_ERROR;
				}
			}
		}
	}

	return result;
}

int mod_matrix_controller_initialise(config_setting_t *config)
{
	if (parse_matrix_controller_settings(config) == RESULT_ERROR)
	{
		return RESULT_ERROR;
	}

	// Turn off all Launchpad LEDs.
	midi_send(MIDI_ALL_DEVICES, 0xb0, midi_channel, 0, 0);

	return RESULT_OK;
}

void mod_matrix_controller_process_midi(int device_handle, int channel, unsigned char event_type, unsigned char data0, unsigned char data1)
{
	if (channel == midi_channel && event_type == 0x90 && data1 == 0x7f)
	{
		int row = (data0 - MOD_MATRIX_CONTROL_NOTE_BASE) / MOD_MATRIX_CONTROL_NOTE_STRIDE;
		int column = (data0 - MOD_MATRIX_CONTROL_NOTE_BASE) % MOD_MATRIX_CONTROL_NOTE_STRIDE;

		if (row < row_count && column < column_count)
		{
			if (mod_matrix_toggle_connection(column_names[column], row_names[row]) == MOD_MATRIX_CONNECTED)
			{
				midi_send(device_handle, 0x90, midi_channel, data0, 0x3c);
			}
			else
			{
				midi_send(device_handle, 0x90, midi_channel, data0, 0x0c);
			}
		}
	}
}

void mod_matrix_controller_deinitialise()
{
	// Turn off all Launchpad LEDs.
	midi_send(MIDI_ALL_DEVICES, 0xb0, midi_channel, 0, 0);
}
