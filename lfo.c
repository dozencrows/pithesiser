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
 * lfo.c
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#include "lfo.h"
#include <math.h>
#include "fixed_point_math.h"

void lfo_init(lfo_t *lfo)
{
	osc_init(&lfo->oscillator);
	lfo->oscillator.waveform = LFO_PROCEDURAL_SINE;
	lfo->oscillator.frequency = 1 * FIXED_ONE;
	lfo->oscillator.level = SHRT_MAX;
	lfo->value = 0;
}

void lfo_reset(lfo_t *lfo)
{
	lfo->oscillator.phase_accumulator = 0;
}

void lfo_update(lfo_t *lfo, int buffer_samples)
{
	osc_mid_output(&lfo->oscillator, &lfo->value, buffer_samples);
}
