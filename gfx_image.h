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
