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
 * waveform.h
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#ifndef WAVEFORM_H_
#define WAVEFORM_H_

#include "system_constants.h"

typedef enum
{
	WAVETABLE_SINE = 0,
	WAVETABLE_SAW,
	WAVETABLE_SAW_BL,
	WAVETABLE_SINE_LINEAR,
	WAVETABLE_SAW_LINEAR,
	WAVETABLE_SAW_LINEAR_BL,

	PROCEDURAL_SINE,
	PROCEDURAL_SAW,

	LFO_PROCEDURAL_SINE,
	LFO_PROCEDURAL_SAW_DOWN,
	LFO_PROCEDURAL_SAW_UP,
	LFO_PROCEDURAL_TRIANGLE,
	LFO_PROCEDURAL_SQUARE,
	LFO_PROCEDURAL_HALFSAW_DOWN,
	LFO_PROCEDURAL_HALFSAW_UP,
	LFO_PROCEDURAL_HALFSINE,
	LFO_PROCEDURAL_HALFTRIANGLE,

	WAVE_FIRST_AUDIBLE =	WAVETABLE_SINE,
	WAVE_LAST_AUDIBLE = 	PROCEDURAL_SAW,
	WAVE_FIRST_LFO = 		LFO_PROCEDURAL_SINE,
	WAVE_LAST_LFO =			LFO_PROCEDURAL_HALFTRIANGLE,

	WAVE_COUNT
} waveform_type_t;

extern void waveform_initialise();

#endif /* WAVEFORM_H_ */
