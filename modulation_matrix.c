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
 * modulation_matrix.c
 *
 *  Created on: 31 Dec 2013
 *      Author: ntuckett
 */

#include "modulation_matrix.h"
#include <memory.h>
#include "system_constants.h"
#include "logging.h"

#define MOD_MATRIX_MAX_CONNECTIONS	(MOD_MATRIX_MAX_SOURCES * MOD_MATRIX_MAX_SINKS)
#define MAX_MOD_MATRIX_CALLBACKS	4

static int source_count	= 0;
static int sink_count = 0;

mod_matrix_source_t* 	sources[MOD_MATRIX_MAX_SOURCES];
mod_matrix_sink_t*		sinks[MOD_MATRIX_MAX_SINKS];

typedef struct mod_matrix_connection_t
{
	mod_matrix_source_t*	source;
	mod_matrix_sink_t*		sink;
} mod_matrix_connection_t;

mod_matrix_connection_t	connections[MOD_MATRIX_MAX_CONNECTIONS];

typedef struct mod_matrix_callback_info_t mod_matrix_callback_info_t;
typedef struct mod_matrix_callback_info_t
{
	mod_matrix_callback_t		callback;
	void*					data;
	mod_matrix_callback_info_t*	next_callback;
} mod_matrix_callback_info_t;

static mod_matrix_callback_info_t* mod_matrix_callback_head = NULL;
static mod_matrix_callback_info_t* mod_matrix_callback_free = NULL;
static mod_matrix_callback_info_t mod_matrix_callback_info[MAX_MOD_MATRIX_CALLBACKS];

void mod_matrix_initialise()
{
	source_count	= 0;
	sink_count		= 0;
	memset(connections, 0, sizeof(connections));

	for (int i = 0; i < MAX_MOD_MATRIX_CALLBACKS; i++)
	{
		mod_matrix_callback_info[i].next_callback = mod_matrix_callback_free;
		mod_matrix_callback_free = mod_matrix_callback_info + i;
	}
}

void mod_matrix_init_source(const char* name, generate_mod_matrix_value_t generate_value, get_mod_matrix_value_t get_value, mod_matrix_source_t* source)
{
	strncpy(source->name, name, MOD_MATRIX_MAX_NAME_LEN);
	source->name[MOD_MATRIX_MAX_NAME_LEN] = 0;
	source->generate_value = generate_value;
	source->get_value = get_value;
}

void mod_matrix_init_sink(const char* name, base_update_t base_update, model_update_t model_update, mod_matrix_sink_t* sink)
{
	strncpy(sink->name, name, MOD_MATRIX_MAX_NAME_LEN);
	sink->name[MOD_MATRIX_MAX_NAME_LEN] = 0;
	sink->base_update = base_update;
	sink->model_update = model_update;
}

int mod_matrix_add_source(mod_matrix_source_t* source)
{
	if (source_count < MOD_MATRIX_MAX_SOURCES)
	{
		sources[source_count++] = source;
		return RESULT_OK;
	}
	else
	{
		LOG_ERROR("Mod matrix source limit of %d reached", MOD_MATRIX_MAX_SOURCES);
		return RESULT_ERROR;
	}
}

int mod_matrix_add_sink(mod_matrix_sink_t* sink)
{
	if (sink_count < MOD_MATRIX_MAX_SINKS)
	{
		sinks[sink_count++] = sink;
		return RESULT_OK;
	}
	else
	{
		LOG_ERROR("Mod matrix sink limit of %d reached", MOD_MATRIX_MAX_SINKS);
		return RESULT_ERROR;
	}
}

void mod_matrix_add_callback(mod_matrix_callback_t callback, void* callback_data)
{
	if (mod_matrix_callback_free != NULL)
	{
		mod_matrix_callback_info_t* callback_info = mod_matrix_callback_free;
		mod_matrix_callback_free = mod_matrix_callback_free->next_callback;

		callback_info->callback 		= callback;
		callback_info->data 			= callback_data;
		callback_info->next_callback 	= mod_matrix_callback_head;

		mod_matrix_callback_head = callback_info;
	}
	else
	{
		LOG_ERROR("No voice callbacks available");
	}
}

void mod_matrix_remove_callback(mod_matrix_callback_t callback)
{
	mod_matrix_callback_info_t* callback_info = mod_matrix_callback_head;
	mod_matrix_callback_info_t** last_link = &mod_matrix_callback_head;

	while (callback_info != NULL)
	{
		if (callback_info->callback == callback)
		{
			*last_link = callback_info->next_callback;
			callback_info->callback 		= NULL;
			callback_info->data				= NULL;
			callback_info->next_callback 	= mod_matrix_callback_free;

			mod_matrix_callback_free = callback_info;
			break;
		}
		else
		{
			last_link = &callback_info->next_callback;
			callback_info = callback_info->next_callback;
		}
	}
}

void mod_matrix_make_callback(mod_matrix_event_t event, mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	mod_matrix_callback_info_t* callback_info = mod_matrix_callback_head;

	while (callback_info != NULL)
	{
		callback_info->callback(event, source, sink, callback_info->data);
		callback_info = callback_info->next_callback;
	}
}

mod_matrix_source_t* find_source(const char* source_name)
{
	for (int i = 0; i < source_count; i++)
	{
		if (strcmp(source_name, sources[i]->name) == 0)
		{
			return sources[i];
		}
	}

	return NULL;
}

mod_matrix_sink_t* find_sink(const char* sink_name)
{
	for (int i = 0; i < sink_count; i++)
	{
		if (strcmp(sink_name, sinks[i]->name) == 0)
		{
			return sinks[i];
		}
	}

	return NULL;
}

mod_matrix_connection_t* find_free_connection()
{
	for (int i = 0; i < MOD_MATRIX_MAX_CONNECTIONS; i++)
	{
		if (connections[i].source == NULL && connections[i].sink == NULL)
		{
			return connections + i;
		}
	}

	return NULL;
}

mod_matrix_connection_t* find_connection(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	for (int i = 0; i < MOD_MATRIX_MAX_CONNECTIONS; i++)
	{
		if (connections[i].source == source && connections[i].sink == sink)
		{
			return connections + i;
		}
	}

	return NULL;
}

mod_matrix_connection_t* connect(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	mod_matrix_connection_t* connection = find_free_connection();

	if (connection != NULL)
	{
		connection->source 	= source;
		connection->sink	= sink;
		mod_matrix_make_callback(MOD_MATRIX_EVENT_CONNECTION, source, sink);
	}

	return connection;
}

void disconnect(mod_matrix_connection_t* connection)
{
	mod_matrix_make_callback(MOD_MATRIX_EVENT_DISCONNECTION, connection->source, connection->sink);
	connection->source 	= NULL;
	connection->sink	= NULL;
}

int mod_matrix_connect(const char* source_name, const char* sink_name)
{
	mod_matrix_source_t* source = find_source(source_name);
	if (source == NULL)
	{
		LOG_ERROR("Unknown source %s for connection", source_name);
		return RESULT_ERROR;
	}

	mod_matrix_sink_t* sink = find_sink(sink_name);
	if (sink == NULL)
	{
		LOG_ERROR("Unknown sink %s for connection", sink_name);
		return RESULT_ERROR;
	}

	if (find_connection(source, sink) == NULL)
	{
		if (connect(source, sink) == NULL)
		{
			LOG_ERROR("No free connections for %s-%s", source_name, sink_name);
			return RESULT_ERROR;
		}
		else
		{
			return RESULT_OK;
		}
	}
	else
	{
		LOG_WARN("Connection exists for %s-%s", source_name, sink_name);
		return RESULT_ERROR;
	}
}

int mod_matrix_disconnect(const char* source_name, const char* sink_name)
{
	mod_matrix_source_t* source = find_source(source_name);
	if (source == NULL)
	{
		LOG_ERROR("Unknown source %s for disconnection", source_name);
		return RESULT_ERROR;
	}

	mod_matrix_sink_t* sink = find_sink(sink_name);
	if (sink == NULL)
	{
		LOG_ERROR("Unknown sink %s for disconnection", sink_name);
		return RESULT_ERROR;
	}

	mod_matrix_connection_t* connection = find_connection(source, sink);

	if (connection != NULL)
	{
		disconnect(connection);
		return RESULT_OK;
	}
	else
	{
		LOG_WARN("No connection exists for %s-%s", source_name, sink_name);
		return RESULT_ERROR;
	}
}

void mod_matrix_disconnect_source(const char* source_name)
{
	mod_matrix_source_t* source = find_source(source_name);
	if (source == NULL)
	{
		LOG_ERROR("Unknown source %s for disconnection", source_name);
	}
	else
	{
		for (int i = 0; i < MOD_MATRIX_MAX_CONNECTIONS; i++)
		{
			if (connections[i].source == source)
			{
				disconnect(connections + i);
			}
		}
	}
}

int mod_matrix_toggle_connection(const char* source_name, const char* sink_name)
{
	mod_matrix_source_t* source = find_source(source_name);
	if (source == NULL)
	{
		LOG_ERROR("Unknown source %s for connection", source_name);
		return MOD_MATRIX_DISCONNECTED;
	}

	mod_matrix_sink_t* sink = find_sink(sink_name);
	if (sink == NULL)
	{
		LOG_ERROR("Unknown sink %s for connection", sink_name);
		return MOD_MATRIX_DISCONNECTED;
	}

	mod_matrix_connection_t* connection = find_connection(source, sink);

	if (connection != NULL)
	{
		disconnect(connection);
		return MOD_MATRIX_DISCONNECTED;
	}
	else
	{
		connect(source, sink);
		return MOD_MATRIX_CONNECTED;
	}
}

void mod_matrix_update(void* data)
{
	for (int i = 0; i < source_count; i++)
	{
		if (sources[i]->generate_value != NULL)
		{
			sources[i]->generate_value(sources[i], data);
		}
	}

	for (int i = 0; i < sink_count; i++)
	{
		if (sinks[i]->base_update != NULL)
		{
			sinks[i]->base_update(sinks[i], data);
		}
	}

	mod_matrix_connection_t* connection = connections;

	for (int i = 0; i < MOD_MATRIX_MAX_CONNECTIONS; i++, connection++)
	{
		if (connection->source != NULL && connection->sink != NULL)
		{
			connection->sink->model_update(connection->source, connection->sink);
		}
	}
}

void mod_matrix_iterate_connections(void* data, connection_callback_t callback)
{
	mod_matrix_connection_t* connection = connections;

	for (int i = 0; i < MOD_MATRIX_MAX_CONNECTIONS; i++, connection++)
	{
		if (connection->source != NULL && connection->sink != NULL)
		{
			callback(data, connection->source, connection->sink);
		}
	}
}
