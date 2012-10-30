/*
 * midi.h
 *
 *  Created on: 30 Oct 2012
 *      Author: ntuckett
 */

#ifndef MIDI_H_
#define MIDI_H_

#define MIDI_MAX_CONTROLLER_VALUE	127

extern int midi_initialise(const char* device_name);
extern void midi_deinitialise();
extern int midi_get_controller_value(int channel_index, int controller_index);

#endif /* MIDI_H_ */
