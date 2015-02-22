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
 * mixer.h
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#ifndef MIXER_H_
#define MIXER_H_

#include "system_constants.h"

extern void copy_mono_to_stereo(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest);
extern void mixdown_mono_to_stereo(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest);

extern void copy_mono_to_stereo_asm(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest);
extern void mixdown_mono_to_stereo_asm(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest);

#endif /* MIXER_H_ */
