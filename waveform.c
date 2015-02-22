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
 * waveform.c
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#include "waveform.h"
#include "waveform_internal.h"
#include "waveform_wavetable.h"
#include "waveform_procedural.h"

waveform_generator_t generators[WAVE_COUNT];

void waveform_initialise()
{
	init_wavetables();
	init_wavetable_generator(WAVETABLE_SINE, &generators[WAVETABLE_SINE]);
	init_wavetable_generator(WAVETABLE_SAW, &generators[WAVETABLE_SAW]);
	init_wavetable_generator(WAVETABLE_SAW_BL, &generators[WAVETABLE_SAW_BL]);
	init_wavetable_generator(WAVETABLE_SINE_LINEAR, &generators[WAVETABLE_SINE_LINEAR]);
	init_wavetable_generator(WAVETABLE_SAW_LINEAR, &generators[WAVETABLE_SAW_LINEAR]);
	init_wavetable_generator(WAVETABLE_SAW_LINEAR_BL, &generators[WAVETABLE_SAW_LINEAR_BL]);

	init_procedural_generator(PROCEDURAL_SINE, &generators[PROCEDURAL_SINE]);
	init_procedural_generator(PROCEDURAL_SAW, &generators[PROCEDURAL_SAW]);
	init_procedural_generator(LFO_PROCEDURAL_SINE, &generators[LFO_PROCEDURAL_SINE]);
	init_procedural_generator(LFO_PROCEDURAL_SAW_DOWN, &generators[LFO_PROCEDURAL_SAW_DOWN]);
	init_procedural_generator(LFO_PROCEDURAL_SAW_UP, &generators[LFO_PROCEDURAL_SAW_UP]);
	init_procedural_generator(LFO_PROCEDURAL_TRIANGLE, &generators[LFO_PROCEDURAL_TRIANGLE]);
	init_procedural_generator(LFO_PROCEDURAL_SQUARE, &generators[LFO_PROCEDURAL_SQUARE]);
	init_procedural_generator(LFO_PROCEDURAL_HALFSAW_DOWN, &generators[LFO_PROCEDURAL_HALFSAW_DOWN]);
	init_procedural_generator(LFO_PROCEDURAL_HALFSAW_UP, &generators[LFO_PROCEDURAL_HALFSAW_UP]);
	init_procedural_generator(LFO_PROCEDURAL_HALFSINE, &generators[LFO_PROCEDURAL_HALFSINE]);
	init_procedural_generator(LFO_PROCEDURAL_HALFTRIANGLE, &generators[LFO_PROCEDURAL_HALFTRIANGLE]);
}
