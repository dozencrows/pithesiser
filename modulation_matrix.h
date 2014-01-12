/*
 * modulation_matrix.h
 *
 *  Created on: 31 Dec 2013
 *      Author: ntuckett
 */

#ifndef MODULATION_MATRIX_H_
#define MODULATION_MATRIX_H_

#define MOD_MATRIX_MAX_NAME_LEN		63
#define MOD_MATRIX_PRECISION		15
#define MOD_MATRIX_MAX_SOURCES		8
#define MOD_MATRIX_MAX_SINKS		8

#define MOD_MATRIX_ONE			(1 << MOD_MATRIX_PRECISION)
#define MOD_MATRIX_MASK			(MOD_MATRIX_ONE - 1)

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

extern void mod_matrix_initialise();
extern void mod_matrix_init_source(const char* name, generate_mod_matrix_value_t generate_value, get_mod_matrix_value_t get_value, mod_matrix_source_t* source);
extern void mod_matrix_init_sink(const char* name, base_update_t base_update, model_update_t model_update, mod_matrix_sink_t* sink);

extern int mod_matrix_add_source(mod_matrix_source_t* source);
extern int mod_matrix_add_sink(mod_matrix_sink_t* sink);
extern int mod_matrix_connect(const char* source_name, const char* sink_name);
extern int mod_matrix_disconnect(const char* source_name, const char* sink_name);
extern void mod_matrix_disconnect_source(const char* source_name);
extern int mod_matrix_toggle_connection(const char* source_name, const char* sink_name);
extern void mod_matrix_iterate_connections(void* data, connection_callback_t callback);
extern void mod_matrix_update(void* data);

#endif /* MODULATION_MATRIX_H_ */
