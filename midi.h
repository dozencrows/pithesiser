/*
 * midi.h
 *
 *  Created on: 30 Oct 2012
 *      Author: ntuckett
 */

#ifndef MIDI_H_
#define MIDI_H_

#include "system_constants.h"

#define MIDI_MAX_CONTROLLER_VALUE	127
#define MIDI_MID_CONTROLLER_VALUE	63
#define MIDI_MIDDLE_C				60

#define MIDI_EVENT_DATA_SIZE	3
typedef struct midi_event_t
{
	unsigned char type;
	unsigned char data[MIDI_EVENT_DATA_SIZE];
} midi_event_t;

extern int midi_initialise(const char* device1_name, const char* device2_name);
extern void midi_deinitialise();
extern int midi_get_controller_changed(int channel_index, int controller_index);
extern int midi_get_controller_value(int channel_index, int controller_index);
extern fixed_t midi_get_note_frequency(int midi_note);
extern fixed_t midi_get_note_wavelength_samples(int midi_note);
extern int midi_get_event_count();
extern midi_event_t *midi_pop_event(midi_event_t *event);

#endif /* MIDI_H_ */
