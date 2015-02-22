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
#define MAX_MIDI_DEVICES			16
#define MIDI_ALL_DEVICES			-1

#define MIDI_EVENT_DATA_SIZE	3
typedef struct midi_event_t
{
	int				device_handle;
	unsigned char 	type;
	unsigned char 	data[MIDI_EVENT_DATA_SIZE];
} midi_event_t;

extern int midi_initialise(int device_count, const char** device_name);
extern void midi_deinitialise();
extern int midi_get_raw_controller_changed(int channel_index, int controller_index);
extern int midi_get_raw_controller_value(int channel_index, int controller_index);
extern fixed_t midi_get_note_frequency(int midi_note);
extern fixed_t midi_get_note_wavelength_samples(int midi_note);
extern int midi_get_event_count();
extern midi_event_t *midi_pop_event(midi_event_t *event);
extern void midi_send_sysex(const char *sysex_message, size_t message_length);
extern void midi_send(int device_handle, unsigned char command, unsigned char channel, unsigned char data0, unsigned char data1);

#endif /* MIDI_H_ */
