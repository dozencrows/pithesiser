/*
 * midi_controller.h
 *
 *  Created on: 11 Feb 2013
 *      Author: ntuckett
 */

#ifndef MIDI_CONTROLLER_H_
#define MIDI_CONTROLLER_H_

#include "setting.h"

typedef enum
{
	NONE = -1,
	CONTINUOUS,
	CONTINUOUS_WITH_HELD,
	CONTINUOUS_RELATIVE,
	CONTINUOUS_RELATIVE_WITH_HELD,
	TOGGLE,
	EVENT
} controller_type_t;

typedef struct midi_controller_t
{
	const char*					name;
	controller_type_t			type;
	int							midi_channel;
	int							midi_cc[2];
	struct midi_controller_t*	indexer_control;
	int							indexer_value;
	union
	{
		struct
		{
			int			min;
			int			max;
		} midi_range;
		int				midi_threshold;
	};
	int					output_min;
	int					output_max;
	int					output_held;
	int					delta_scale;
	int					last_output;
} midi_controller_t;

extern void midi_controller_create(midi_controller_t* controller, const char* name);
extern void midi_controller_init(midi_controller_t* controller);
extern int midi_controller_update(midi_controller_t* controller);
extern int midi_controller_read(midi_controller_t* controller);
extern int midi_controller_update_and_read(midi_controller_t* controller, int* value);
extern void midi_controller_set_setting(midi_controller_t* controller, setting_t* setting);
extern midi_controller_t* midi_controller_new_index_control(const char* name);
extern midi_controller_t* midi_controller_find_index_control(const char* name);
extern void midi_controller_update_index_controls();

#endif /* MIDI_CONTROLLER_H_ */
