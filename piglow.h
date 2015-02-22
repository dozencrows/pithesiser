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
 * piglow.h
 *
 *  Created on: 23 Dec 2013
 *      Author: ntuckett
 */

#ifndef PIGLOW_H_
#define PIGLOW_H_

typedef struct voice_t voice_t;
typedef struct config_setting_t config_setting_t;

extern void piglow_initialise(config_setting_t* config);
extern void piglow_update(voice_t* voice, int voice_count);
extern void piglow_deinitialise();

#endif /* PIGLOW_H_ */
