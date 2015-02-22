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
 * oscillator.h
 *
 *  Created on: 31 Oct 2012
 *      Author: ntuckett
 *
 *  Assumptions:
 *  	2 channels
 *  	Sample format is signed 16-bit little endian
 *  	Sample rate is 44100Hz
 */

#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include <sys/types.h>
#include "system_constants.h"
#include "waveform.h"

typedef struct oscillator_t
{
	waveform_type_t		waveform;
	fixed_t				frequency;
	fixed_t				phase_accumulator;
	int32_t				level;
	int32_t				last_level;
} oscillator_t;

extern void osc_init(oscillator_t* osc);
extern void osc_output(oscillator_t* osc, sample_t *sample_data, int sample_count);
extern void osc_mix_output(oscillator_t* osc, sample_t *sample_data, int sample_count);
extern void osc_mid_output(oscillator_t* osc, sample_t *sample_data, int sample_count);

#endif /* OSCILLATOR_H_ */
