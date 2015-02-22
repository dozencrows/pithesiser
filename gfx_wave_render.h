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
 * gfx_wave_render.h
 *
 *  Created on: 31 Dec 2012
 *      Author: ntuckett
 */

#ifndef GFX_WAVE_RENDER_H_
#define GFX_WAVE_RENDER_H_

#include <sys/types.h>
#include "VG/openvg.h"
#include "system_constants.h"
#include "gfx_object.h"

typedef struct wave_renderer_t
{
	gfx_object_t gfx_object;
	int	x, y;
	int width, height;
	int tuned_wavelength_fx;
	int amplitude_scale;
	VGfloat	background_colour[4];
	VGfloat	line_colour[4];
	VGfloat line_width;
	VGint line_cap_style;
	VGint line_join_style;
	VGint rendering_quality;
} wave_renderer_t;

extern void gfx_wave_render_initialise();
extern void gfx_wave_render_deinitialise();

extern wave_renderer_t *gfx_wave_renderer_create(object_id_t id);
extern void gfx_wave_render_wavelength(wave_renderer_t *renderer, fixed_t wavelength_samples);
extern void gfx_wave_renderer_destroy(wave_renderer_t *waveform_renderer);

#endif /* GFX_WAVE_RENDER_H_ */
