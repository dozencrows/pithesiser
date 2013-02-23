/*
 * midi_controller.h
 *
 *  Created on: 11 Feb 2013
 *      Author: ntuckett
 */

#ifndef MIDI_CONTROLLER_H_
#define MIDI_CONTROLLER_H_

typedef enum
{
	NONE = -1,
	CONTINUOUS,
	CONTINUOUS_WITH_HELD,
	TOGGLE,
	EVENT
} controller_type_t;

typedef struct midi_controller_t
{
	controller_type_t	type;
	int					midi_channel;
	int					midi_cc[2];
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
	int					last_output;
} midi_controller_t;

extern void midi_controller_create(midi_controller_t *controller);
extern void midi_controller_init(midi_controller_t *controller);
extern int midi_controller_update(midi_controller_t *controller, int *value);

#endif /* MIDI_CONTROLLER_H_ */
