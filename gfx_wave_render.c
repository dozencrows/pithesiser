/*
 * gfx_wave_render.c
 *
 *  Created on: 31 Dec 2012
 *      Author: ntuckett
 */

#include "gfx_wave_render.h"
#include <stdlib.h>
#include "gfx_event.h"
#include "gfx_event_types.h"
#include "VG/openvg.h"

#define WAVE_IMAGE_WIDTH		1024
#define WAVE_IMAGE_HEIGHT		512
#define WAVE_IMAGE_PIXEL_STRIDE	2
#define WAVE_IMAGE_ROW_STRIDE	(WAVE_IMAGE_PIXEL_STRIDE * WAVE_IMAGE_WIDTH)
#define WAVE_IMAGE_SCALE		129		// roughly 32767 divided by 256 with a safety margin
#define WAVE_IMAGE_ZERO			(WAVE_IMAGE_HEIGHT / 2)
#define WAVE_COLOUR				0x03e0
#define WAVE_CLEAR_COLOUR		0x0000

#define BYTES_PER_CHANNEL	(sizeof(int16_t))
#define CHANNELS_PER_SAMPLE	2
#define BYTES_PER_SAMPLE	(BYTES_PER_CHANNEL * CHANNELS_PER_SAMPLE)

//
// Needs:
//	* Image buffer in which to build up waveform display
//	  * Sample-per-pixel horizontally
//	  * Parameters:
//		* Image format (best match display format)
//		* Width
//		* Height
//	* Event to render waveform chunk into buffer
//	* Means to trigger rendering of image to screen
//	  * Using vgWritePixels - immediately updates screen
//
// Alternative ideas:
//	* One buffer per chunk: rendered to screen immediately after render to buffer
//	* Use vgSetPixels to render (requires image object and intermediate upload)
//  * Use vgDrawImage to render (requires image object and intermediate upload, greater power over image appearance)
//
// Future thoughts:
//	* Stereo!

static char *pixel_buffer	= NULL;
static int buffer_render_x	= 0;

static void fill_column(int column, int16_t colour, size_t start_y, int height)
{
	char *column_ptr = pixel_buffer + column * WAVE_IMAGE_PIXEL_STRIDE + (WAVE_IMAGE_HEIGHT - start_y - 1) * WAVE_IMAGE_ROW_STRIDE;

	while (height-- > 0)
	{
		*(int16_t*)column_ptr = colour;
		column_ptr -= WAVE_IMAGE_ROW_STRIDE;
	}
}

static void render_test_pattern()
{
	for(int i = 0; i < WAVE_IMAGE_HEIGHT; i++)
	{
		fill_column(i, WAVE_COLOUR, 0, i);
	}
}

// Beware: called from GFX thread!
static void wave_event_handler(gfx_event_t *event)
{
	if (buffer_render_x < WAVE_IMAGE_WIDTH)
	{
		size_t column_count = event->size / BYTES_PER_SAMPLE;
		int16_t *sample_data = (int16_t*) event->ptr;

		while (column_count-- > 0)
		{
			size_t wave_y = WAVE_IMAGE_ZERO - ((*sample_data) / WAVE_IMAGE_SCALE);
			size_t bar_start_y;
			size_t bar_end_y;

			if (*sample_data >= 0)
			{
				bar_start_y = wave_y;
				bar_end_y = WAVE_IMAGE_ZERO;
			}
			else
			{
				bar_start_y = WAVE_IMAGE_ZERO;
				bar_end_y = wave_y;
			}

			fill_column(buffer_render_x, WAVE_CLEAR_COLOUR, 0, bar_start_y);
			fill_column(buffer_render_x, WAVE_COLOUR, bar_start_y, bar_end_y - bar_start_y);
			fill_column(buffer_render_x, WAVE_CLEAR_COLOUR, bar_end_y, WAVE_IMAGE_HEIGHT - bar_end_y);

			sample_data += CHANNELS_PER_SAMPLE;
			buffer_render_x++;
			if (buffer_render_x >= WAVE_IMAGE_WIDTH)
			{
				break;
			}
		}
	}

	free(event->ptr);
}

static void silence_event_handler(gfx_event_t *event)
{
	if (buffer_render_x < WAVE_IMAGE_WIDTH)
	{
		size_t column_count = event->size / BYTES_PER_SAMPLE;

		while (column_count-- > 0)
		{
			fill_column(buffer_render_x, WAVE_CLEAR_COLOUR, 0, WAVE_IMAGE_HEIGHT);
			buffer_render_x++;
			if (buffer_render_x >= WAVE_IMAGE_WIDTH)
			{
				break;
			}
		}
	}
}

static void swap_event_handler(gfx_event_t *event)
{
	vgWritePixels(pixel_buffer, WAVE_IMAGE_ROW_STRIDE, VG_sRGB_565, 0, 0, WAVE_IMAGE_WIDTH, WAVE_IMAGE_HEIGHT);
	buffer_render_x = 0;
}

void gfx_wave_render_initialise()
{
	gfx_register_event_handler(GFX_EVENT_WAVE, wave_event_handler);
	gfx_register_event_handler(GFX_EVENT_SILENCE, silence_event_handler);
	gfx_register_event_handler(GFX_EVENT_BUFFERSWAP, swap_event_handler);

	buffer_render_x = 0;

	pixel_buffer = (char*) malloc(WAVE_IMAGE_ROW_STRIDE * WAVE_IMAGE_HEIGHT);
}

void gfx_wave_render_deinitialise()
{
	free(pixel_buffer);
	pixel_buffer = NULL;
}
