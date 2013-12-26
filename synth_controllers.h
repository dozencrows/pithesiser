/*
 * synth_controllers.h
 *
 *  Created on: 11 Feb 2013
 *      Author: ntuckett
 */

#ifndef SYNTH_CONTROLLERS_H_
#define SYNTH_CONTROLLERS_H_

#include "midi_controller.h"

typedef struct config_setting_t config_setting_t;
typedef struct synth_model_t synth_model_t;

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

extern midi_controller_t exit_controller;
extern midi_controller_t profile_controller;
extern midi_controller_t screenshot_controller;

extern int synth_controllers_initialise(int controller_channel, config_setting_t *config);
extern int synth_controllers_save(const char* file_path);
extern int synth_controllers_load(const char* file_path, synth_model_t* synth_model);
extern void process_synth_controllers(synth_model_t* synth_model);

#endif /* SYNTH_CONTROLLERS_H_ */
