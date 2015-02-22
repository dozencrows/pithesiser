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

extern midi_controller_t oscilloscope_controller;
extern midi_controller_t exit_controller;
extern midi_controller_t profile_controller;
extern midi_controller_t screenshot_controller;

extern int synth_controllers_initialise(int controller_channel, config_setting_t *config);
extern int synth_controllers_save(const char* file_path);
extern int synth_controllers_load(const char* file_path, synth_model_t* synth_model);
extern void process_synth_controllers(synth_model_t* synth_model);

#endif /* SYNTH_CONTROLLERS_H_ */
