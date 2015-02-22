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
 * modulation_matrix_controller.h
 *
 *  Created on: 31 Dec 2013
 *      Author: ntuckett
 */

#ifndef MODULATION_MATRIX_CONTROLLER_H_
#define MODULATION_MATRIX_CONTROLLER_H_

typedef struct config_setting_t config_setting_t;

extern int mod_matrix_controller_initialise(config_setting_t* config, config_setting_t* patch);
extern void mod_matrix_controller_process_midi(int device_handle, int channel, unsigned char event_type, unsigned char data0, unsigned char data1);
extern void mod_matrix_controller_save(config_setting_t* patch);
extern void mod_matrix_controller_deinitialise();

#endif /* MODULATION_MATRIX_CONTROLLER_H_ */
