/*
 * synth_controllers.h
 *
 *  Created on: 11 Feb 2013
 *      Author: ntuckett
 */

#ifndef SYNTH_CONTROLLERS_H_
#define SYNTH_CONTROLLERS_H_

#include "midi_controller.h"

#define LFO_STATE_OFF			0
#define LFO_STATE_VOLUME		1
#define LFO_STATE_PITCH			2

extern midi_controller_t master_volume_controller;
extern midi_controller_t waveform_controller;
extern midi_controller_t oscilloscope_controller;

extern midi_controller_t envelope_attack_time_controller;
extern midi_controller_t envelope_attack_level_controller;
extern midi_controller_t envelope_decay_time_controller;
extern midi_controller_t envelope_decay_level_controller;
extern midi_controller_t envelope_sustain_time_controller;
extern midi_controller_t envelope_release_time_controller;

extern midi_controller_t lfo_state_controller;
extern midi_controller_t lfo_waveform_controller;
extern midi_controller_t lfo_level_controller;
extern midi_controller_t lfo_frequency_controller;

extern midi_controller_t filter_state_controller;
extern midi_controller_t filter_frequency_controller;
extern midi_controller_t filter_q_controller;

extern void synth_controllers_initialise(int midi_channel);

#endif /* SYNTH_CONTROLLERS_H_ */
