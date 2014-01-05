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

void mod_matrix_initialise()
{
	source_count	= 0;
	sink_count		= 0;
	memset(connections, 0, sizeof(connections));
}

void mod_matrix_init_source(const char* name, generate_mod_matrix_value_t generate_value, mod_matrix_source_t* source)
{
	strncpy(source->name, name, MOD_MATRIX_MAX_NAME_LEN);
	source->name[MOD_MATRIX_MAX_NAME_LEN] = 0;
	source->generate_value = generate_value;
	source->value = 0;
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
	}

	return connection;
}

void disconnect(mod_matrix_connection_t* connection)
{
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

void mod_matrix_update()
{
	for (int i = 0; i < source_count; i++)
	{
		sources[i]->value = sources[i]->generate_value(sources[i]);
	}

	for (int i = 0; i < sink_count; i++)
	{
		sinks[i]->base_update(sinks[i]);
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
