/*
 * modulation_matrix_controller.c
 *
 *  Created on: 31 Dec 2013
 *      Author: ntuckett
 */

#include "modulation_matrix_controller.h"
#include <libconfig.h>
#include <syslog.h>
#include <string.h>
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

static const char* CFG_MOD_MATRIX_CHANNEL 		= "channel";
static const char* CFG_MOD_MATRIX_COLUMNS 		= "columns";
static const char* CFG_MOD_MATRIX_ROWS 			= "rows";
static const char* CFG_MOD_MATRIX_CONNECTIONS 	= "connections";

static unsigned char get_matrix_note_number(const char* source, const char* sink)
{
	int column;
	int row;

	for (column = 0; column < column_count; column++)
	{
		if (strcmp(source, column_names[column]) == 0)
		{
			break;
		}
	}

	if (column == column_count)
	{
		return 0xff;
	}

	for (row = 0; row < row_count; row++)
	{
		if (strcmp(sink, row_names[row]) == 0)
		{
			break;
		}
	}

	if (row == row_count)
	{
		return 0xff;
	}

	return (unsigned char)(MOD_MATRIX_CONTROL_NOTE_BASE + column + row * MOD_MATRIX_CONTROL_NOTE_STRIDE);
}

static int parse_matrix_controller_settings(config_setting_t *config)
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

void mod_matrix_controller_load(config_setting_t* patch)
{
	config_setting_t* connections = config_setting_get_member(patch, CFG_MOD_MATRIX_CONNECTIONS);
	if (connections != NULL)
	{
		for (int i = 0; i < config_setting_length(connections); i++)
		{
			config_setting_t* connection = config_setting_get_elem(connections, i);
			if (connection != NULL)
			{
				const char* source = config_setting_get_string_elem(connection, 0);
				const char* sink = config_setting_get_string_elem(connection, 1);

				if (source != NULL && sink != NULL)
				{
					mod_matrix_connect(source, sink);
					unsigned char midi_note = get_matrix_note_number(source, sink);
					if (midi_note != 0xff)
					{
						midi_send(MIDI_ALL_DEVICES, 0x90, midi_channel, midi_note, 0x3c);
					}
				}
				else if (source == NULL)
				{
					setting_error_report(connection, "Modulation matrix patch data missing source");
				}
				else if (source == NULL)
				{
					setting_error_report(connection, "Modulation matrix patch data missing source");
				}
			}
		}
	}
	else
	{
		setting_error_report(patch, "Patch contains no modulation matrix data");
	}
}

static void mod_matrix_save_callback(void* data, mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	config_setting_t* connections = (config_setting_t*)data;
	config_setting_t* connection = config_setting_add(connections, NULL, CONFIG_TYPE_ARRAY);

	if (connection != NULL)
	{
		config_setting_add(connection, NULL, CONFIG_TYPE_STRING);
		config_setting_add(connection, NULL, CONFIG_TYPE_STRING);

		if (config_setting_set_string_elem(connection, 0, source->name) == NULL || config_setting_set_string_elem(connection, 1, sink->name) == NULL)
		{
			syslog(LOG_ERR, "Could not set modulation matrix entry in patch for %s-%s", source->name, sink->name);
			config_setting_remove_elem(connections, config_setting_index(connection));
		}
	}
	else
	{
		syslog(LOG_ERR, "Could not create modulation matrix entry in patch for %s-%s", source->name, sink->name);
	}
}

void mod_matrix_controller_save(config_setting_t* patch)
{
	config_setting_t* connections = config_setting_add(patch, CFG_MOD_MATRIX_CONNECTIONS, CONFIG_TYPE_LIST);
	if (connections != NULL)
	{
		mod_matrix_iterate_connections(connections, mod_matrix_save_callback);
	}
	else
	{
		syslog(LOG_ERR, "Could not create modulation matrix entry in patch");
	}
}

int mod_matrix_controller_initialise(config_setting_t *config, config_setting_t *patch)
{
	if (parse_matrix_controller_settings(config) == RESULT_ERROR)
	{
		return RESULT_ERROR;
	}

	// Turn off all Launchpad LEDs.
	midi_send(MIDI_ALL_DEVICES, 0xb0, midi_channel, 0, 0);

	if (patch != NULL)
	{
		mod_matrix_controller_load(patch);
	}

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
