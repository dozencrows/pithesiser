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
 * recording.h
 *
 *  Created on: 2 Mar 2013
 *      Author: ntuckett
 */

#ifndef RECORDING_H_
#define RECORDING_H_

#include <libconfig.h>

extern void recording_initialise(config_t *config, int gfx_event_id);
extern void recording_deinitialise();


#endif /* RECORDING_H_ */
