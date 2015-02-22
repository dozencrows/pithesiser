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
 * gfx_setting_renderer.h
 *
 *  Created on: 8 Sep 2013
 *      Author: ntuckett
 */

#ifndef GFX_SETTING_RENDERER_H_
#define GFX_SETTING_RENDERER_H_

#include "VG/openvg.h"
#include "gfx_object.h"
#include "setting.h"

typedef struct setting_renderer_t
{
	gfx_object_t 	gfx_object;
	int				x, y;
	int 			width, height;
	const char*		text;
	VGint 			text_quality;
	VGfloat			text_colour[4];
	VGfloat			background_colour[4];
	VGint 			text_size;
	VGint 			text_x_offset;
	VGint 			text_y_offset;
	setting_t*		setting;
	const char*		format;
} setting_renderer_t;

extern void gfx_setting_render_initialise();
extern void gfx_setting_render_deinitialise();

extern setting_renderer_t* gfx_setting_renderer_create(object_id_t id);
extern void gfx_setting_renderer_destroy(setting_renderer_t* setting_renderer);

#endif /* GFX_SETTING_RENDERER_H_ */
