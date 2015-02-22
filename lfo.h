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
 * lfo.h
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#ifndef LFO_H_
#define LFO_H_

#include "oscillator.h"

#define LFO_MIN_FREQUENCY		(FIXED_ONE / 10)
#define LFO_MAX_FREQUENCY		(20 * FIXED_ONE)

typedef struct lfo_t
{
	oscillator_t 		oscillator;
	sample_t 			value;
} lfo_t;

extern void lfo_init(lfo_t *lfo);
extern void lfo_reset(lfo_t *lfo);
extern void lfo_update(lfo_t *lfo, int buffer_samples);

#endif /* LFO_H_ */
