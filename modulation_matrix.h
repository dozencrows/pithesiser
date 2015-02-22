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
 * modulation_matrix.h
 *
 *  Created on: 31 Dec 2013
 *      Author: ntuckett
 */

#ifndef MODULATION_MATRIX_H_
#define MODULATION_MATRIX_H_

#define MOD_MATRIX_MAX_NAME_LEN		63
#define MOD_MATRIX_MAX_SOURCES		8
#define MOD_MATRIX_MAX_SINKS		8
#define MOD_MATRIX_SINGLE_SOURCE	-1

#define MOD_MATRIX_PRECISION		15
#define MOD_MATRIX_ONE				(1 << MOD_MATRIX_PRECISION)

#define MOD_MATRIX_CONNECTED		1
#define MOD_MATRIX_DISCONNECTED		0

typedef int mod_matrix_value_t;

typedef struct mod_matrix_source_t mod_matrix_source_t;
typedef struct mod_matrix_sink_t mod_matrix_sink_t;

typedef void (*generate_mod_matrix_value_t)(mod_matrix_source_t* source, void* data);
typedef mod_matrix_value_t (*get_mod_matrix_value_t)(mod_matrix_source_t* source, int subsource_id);
typedef void (*base_update_t)(mod_matrix_sink_t* sink, void* data);
typedef void (*model_update_t)(mod_matrix_source_t* source, mod_matrix_sink_t* sink);

typedef void (*connection_callback_t)(void* data, mod_matrix_source_t* source, mod_matrix_sink_t* sink);

struct mod_matrix_source_t
{
	char 						name[MOD_MATRIX_MAX_NAME_LEN + 1];
	generate_mod_matrix_value_t	generate_value;
	get_mod_matrix_value_t		get_value;
};

struct mod_matrix_sink_t
{
	char 			name[MOD_MATRIX_MAX_NAME_LEN + 1];
	base_update_t	base_update;
	model_update_t	model_update;
};

typedef enum mod_matrix_event_t
{
	MOD_MATRIX_EVENT_CONNECTION,
	MOD_MATRIX_EVENT_DISCONNECTION
} mod_matrix_event_t;

typedef void (*mod_matrix_callback_t)(mod_matrix_event_t callback_event, mod_matrix_source_t* source, mod_matrix_sink_t* sink, void* callback_data);

extern void mod_matrix_initialise();
extern void mod_matrix_init_source(const char* name, generate_mod_matrix_value_t generate_value, get_mod_matrix_value_t get_value, mod_matrix_source_t* source);
extern void mod_matrix_init_sink(const char* name, base_update_t base_update, model_update_t model_update, mod_matrix_sink_t* sink);
extern void mod_matrix_add_callback(mod_matrix_callback_t callback, void* callback_data);
extern void mod_matrix_remove_callback(mod_matrix_callback_t callback);

extern int mod_matrix_add_source(mod_matrix_source_t* source);
extern int mod_matrix_add_sink(mod_matrix_sink_t* sink);
extern int mod_matrix_connect(const char* source_name, const char* sink_name);
extern int mod_matrix_disconnect(const char* source_name, const char* sink_name);
extern void mod_matrix_disconnect_source(const char* source_name);
extern int mod_matrix_toggle_connection(const char* source_name, const char* sink_name);
extern void mod_matrix_iterate_connections(void* data, connection_callback_t callback);
extern void mod_matrix_update(void* data);

#endif /* MODULATION_MATRIX_H_ */
