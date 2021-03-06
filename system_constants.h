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
 * system_constants.h
 *
 *  Created on: 12 Jan 2013
 *      Author: ntuckett
 */

#ifndef SYSTEM_CONSTANTS_H_
#define SYSTEM_CONSTANTS_H_

#include <sys/types.h>
#include <limits.h>

#define TRUE						1
#define FALSE						0

#define RESULT_OK					0
#define RESULT_ERROR				-1

#define SYSTEM_SAMPLE_RATE			44100
#define SYSTEM_FRAME_RATE			60

#define FIXED_PRECISION				18
#define FIXED_FRACTION_MASK			0x0003ffff
#define FIXED_ONE					0x00040000
#define FIXED_HALF					0x00020000

typedef int16_t	sample_t;
typedef int32_t	fixed_t;
typedef int64_t	fixed_wide_t;

#define LEVEL_MAX			SHRT_MAX
#define SAMPLE_MAX			SHRT_MAX
#define PAN_MAX				32768
#define BYTES_PER_CHANNEL	(sizeof(sample_t))
#define CHANNELS_PER_SAMPLE	2
#define BYTES_PER_SAMPLE	(BYTES_PER_CHANNEL * CHANNELS_PER_SAMPLE)

#define WAVE_RENDERER_ID			1
#define ENVELOPE_1_RENDERER_ID		2
#define IMAGE_RENDERER_ID			3
#define ENVELOPE_2_RENDERER_ID		4
#define ENVELOPE_3_RENDERER_ID		5
#define MASTER_VOLUME_RENDERER_ID	6
#define MASTER_WAVEFORM_RENDERER_ID	7

#endif /* SYSTEM_CONSTANTS_H_ */
