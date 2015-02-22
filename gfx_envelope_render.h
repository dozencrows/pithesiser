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
 * gfx_envelope_render.h
 *
 *  Created on: 19 Jan 2013
 *      Author: ntuckett
 */

#ifndef GFX_ENVELOPE_RENDER_H_
#define GFX_ENVELOPE_RENDER_H_

#include <sys/types.h>
#include "VG/openvg.h"
#include "gfx_object.h"
#include "envelope.h"

typedef struct envelope_renderer_t
{
	gfx_object_t gfx_object;
	int	x, y;
	int width, height;
	VGfloat	background_colour[4];
	VGfloat	line_colour[4];
	VGfloat line_width;
	VGint line_cap_style;
	VGint line_join_style;
	VGint rendering_quality;
	envelope_t *envelope;
	const char *text;
	VGint text_quality;
	VGfloat	text_colour[4];
} envelope_renderer_t;

extern void gfx_envelope_render_initialise();
extern void gfx_envelope_render_deinitialise();

extern envelope_renderer_t *gfx_envelope_renderer_create(object_id_t id);
extern void gfx_envelope_renderer_destroy(envelope_renderer_t *waveform_renderer);


#endif /* GFX_ENVELOPE_RENDER_H_ */
