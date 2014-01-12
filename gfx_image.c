/*
 * gfx_image.c
 *
 *  Created on: 27 Jan 2013
 *      Author: ntuckett
 */
#include "gfx_image.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "logging.h"
#include "VG/openvg.h"
#include "gfx_event.h"
#include "gfx_event_types.h"

#define PNG_DEBUG 3
#include "libpng/png.h"

#define VG_ERROR_CHECK(s)	{ VGErrorCode error = vgGetError(); if (error != VG_NO_ERROR) LOG_ERROR("VG Error: %d (%s)", error, s); }

void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

typedef struct image_renderer_state_t
{
	int image_width;
	int image_height;
	png_byte color_type;
	png_byte bit_depth;

	png_structp png_ptr;
	png_infop info_ptr;
	int number_of_passes;
	png_byte* image_ptr;
	png_bytep * row_pointers;

	int display_update;
	VGImage image_handle;
} image_renderer_state_t;

typedef struct image_renderer_internal_t
{
	image_renderer_t 		definition;
	image_renderer_state_t	state;
} image_renderer_internal_t;

static void deinit_state(image_renderer_state_t *image_state)
{
	if (image_state->image_ptr != NULL)
	{
		free(image_state->image_ptr);
	}

	if (image_state->row_pointers != NULL)
	{
		free(image_state->row_pointers);
	}

	if (image_state->png_ptr != NULL)
	{
		if (image_state->info_ptr != NULL)
		{
			png_destroy_read_struct(&image_state->png_ptr, &image_state->info_ptr, NULL);
		}
		else
		{
			png_destroy_read_struct(&image_state->png_ptr, NULL, NULL);
		}
	}

	if (image_state->image_handle != 0)
	{
		vgDestroyImage(image_state->image_handle);
	}
	memset(image_state, 0, sizeof(image_renderer_state_t));
}

static int read_png_file(char* file_name, image_renderer_state_t *image_state)
{
	unsigned char header[8]; // 8 is the maximum size that can be checked

	FILE *fp = fopen(file_name, "rb");
	if (!fp)
	{
		deinit_state(image_state);
		return 0;
	}

	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
	{
		deinit_state(image_state);
		return 0;
	}

	/* initialize stuff */
	image_state->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!image_state->png_ptr)
	{
		deinit_state(image_state);
		return 0;
	}

	image_state->info_ptr = png_create_info_struct(image_state->png_ptr);
	if (!image_state->info_ptr)
	{
		deinit_state(image_state);
		return 0;
	}

	if (setjmp(png_jmpbuf(image_state->png_ptr)))
	{
		deinit_state(image_state);
		return 0;
	}

	png_init_io(image_state->png_ptr, fp);
	png_set_sig_bytes(image_state->png_ptr, 8);

	png_read_info(image_state->png_ptr, image_state->info_ptr);

	image_state->image_width = png_get_image_width(image_state->png_ptr, image_state->info_ptr);
	image_state->image_height = png_get_image_height(image_state->png_ptr, image_state->info_ptr);
	image_state->color_type = png_get_color_type(image_state->png_ptr, image_state->info_ptr);
	image_state->bit_depth = png_get_bit_depth(image_state->png_ptr, image_state->info_ptr);

	image_state->number_of_passes = png_set_interlace_handling(image_state->png_ptr);
	png_read_update_info(image_state->png_ptr, image_state->info_ptr);

	/* read file */
	if (setjmp(png_jmpbuf(image_state->png_ptr)))
	{
		deinit_state(image_state);
		return 0;
	}

	image_state->row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * image_state->image_height);
	png_uint_32 row_bytes = png_get_rowbytes(image_state->png_ptr, image_state->info_ptr);
	if (image_state->info_ptr->channels == 3)
	{
		row_bytes += image_state->image_width;
	}

	image_state->image_ptr = (png_byte*) malloc(row_bytes * image_state->image_height);
	for (int y = 0; y < image_state->image_height; y++)
		image_state->row_pointers[image_state->image_height - y - 1] = image_state->image_ptr + y * row_bytes;

	png_read_image(image_state->png_ptr, image_state->row_pointers);

	fclose(fp);

	return 1;
}

static void create_image(image_renderer_internal_t *renderer)
{
	image_renderer_state_t *state = &renderer->state;
	if (state->info_ptr->channels == 3 && state->info_ptr->bit_depth == 8 && state->info_ptr->color_type == PNG_COLOR_TYPE_RGB)
	{
		for (int y = 0; y < state->image_height; y++)
		{
			png_byte* row_ptr = state->row_pointers[y];

			for (int x = state->image_width - 1; x >= 0; x--)
			{
				// Pi is little endian - source data is RGB in memory order, but when treated as pixels is reversed.
				// Written in reverse order to avoid overwrites.
				row_ptr[x*4 + 3] = 255;
				row_ptr[x*4 + 2] = row_ptr[x*3 + 2];
				row_ptr[x*4 + 1] = row_ptr[x*3 + 1];
				row_ptr[x*4 + 0] = row_ptr[x*3 + 0];
			}
		}

		state->image_handle = vgCreateImage(VG_sRGB_565, state->image_width, state->image_height, VG_IMAGE_QUALITY_FASTER);
		VG_ERROR_CHECK("vgCreateImage");
		vgImageSubData(state->image_handle, state->image_ptr, state->image_width * 4, VG_sABGR_8888, 0, 0, state->image_width, state->image_height);
		VG_ERROR_CHECK("vgImageSubData");
	}
	else if (state->info_ptr->channels == 4 && state->info_ptr->bit_depth == 8 && state->info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		state->image_handle = vgCreateImage(VG_sRGBA_5551, state->image_width, state->image_height, VG_IMAGE_QUALITY_FASTER);
		VG_ERROR_CHECK("vgCreateImage");
		vgImageSubData(state->image_handle, state->image_ptr, state->image_width * 4, VG_sABGR_8888, 0, 0, state->image_width, state->image_height);
		VG_ERROR_CHECK("vgImageSubData");
	}
}

static void update_display(image_renderer_internal_t *renderer)
{
	if (renderer->state.image_handle != 0)
	{
		vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
		vgLoadIdentity();
		vgTranslate(renderer->definition.x, renderer->definition.y);
		vgDrawImage(renderer->state.image_handle);
		VG_ERROR_CHECK("vgDrawImage");
	}
}

//--------------------------------------------------------------------------------------------------------------
// Event handlers
//

static void refresh_handler(gfx_event_t *event, gfx_object_t *receiver)
{
	image_renderer_internal_t *image_renderer = (image_renderer_internal_t*)receiver;
	image_renderer->state.display_update = 1;
}

static void swap_handler(gfx_event_t *event, gfx_object_t *receiver)
{
	image_renderer_internal_t *image_renderer = (image_renderer_internal_t*)receiver;
	if (image_renderer->state.display_update)
	{
		update_display(image_renderer);
		image_renderer->state.display_update = 0;
	}
}

static void post_init_handler(gfx_event_t *event, gfx_object_t *receiver)
{
	image_renderer_internal_t *image_renderer = (image_renderer_internal_t*)receiver;
	if (read_png_file(image_renderer->definition.image_file, &image_renderer->state))
	{
		create_image(image_renderer);
		image_renderer->state.display_update = 1;
	}
}

//--------------------------------------------------------------------------------------------------------------
// Public interface
//

void gfx_image_render_initialise()
{

}

void gfx_image_render_deinitialise()
{

}

image_renderer_t *gfx_image_renderer_create(object_id_t id)
{
	image_renderer_internal_t *image_renderer = (image_renderer_internal_t*)calloc(1, sizeof(image_renderer_internal_t));

	gfx_object_t *gfx_object = &image_renderer->definition.gfx_object;
	gfx_object->id = id;
	gfx_register_event_receiver_handler(GFX_EVENT_POSTINIT, post_init_handler, gfx_object);
	gfx_register_event_receiver_handler(GFX_EVENT_REFRESH, refresh_handler, gfx_object);
	gfx_register_event_receiver_handler(GFX_EVENT_BUFFERSWAP, swap_handler, gfx_object);

	return (image_renderer_t*)image_renderer;
}

void gfx_image_renderer_destroy(image_renderer_t *renderer)
{
	image_renderer_internal_t *image_renderer = (image_renderer_internal_t*)renderer;
	deinit_state(&image_renderer->state);
	free(image_renderer);
}
