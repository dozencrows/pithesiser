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
 * gfx_image.h
 *
 *  Created on: 27 Jan 2013
 *      Author: ntuckett
 */

#ifndef GFX_IMAGE_H_
#define GFX_IMAGE_H_

#include "gfx_object.h"

typedef struct image_renderer_t
{
	gfx_object_t gfx_object;
	int	x, y;
	int width, height;
	char *image_file;
} image_renderer_t;

extern void gfx_image_render_initialise();
extern void gfx_image_render_deinitialise();

extern image_renderer_t *gfx_image_renderer_create(object_id_t id);
extern void gfx_image_renderer_destroy(image_renderer_t *image_renderer);


#endif /* GFX_IMAGE_H_ */
