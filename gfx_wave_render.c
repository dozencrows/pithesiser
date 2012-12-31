/*
 * gfx_wave_render.c
 *
 *  Created on: 31 Dec 2012
 *      Author: ntuckett
 */

#include "gfx_wave_render.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <memory.h>
#include "gfx_event.h"
#include "gfx_event_types.h"
#include "VG/openvg.h"

//--------------------------------------------------------------------------------------------------------------
// Common waveform rendering support

#define BYTES_PER_CHANNEL	(sizeof(int16_t))
#define CHANNELS_PER_SAMPLE	2
#define BYTES_PER_SAMPLE	(BYTES_PER_CHANNEL * CHANNELS_PER_SAMPLE)

#define VG_ERROR_CHECK(s)	{ VGErrorCode error = vgGetError(); if (error != VG_NO_ERROR) printf("VG Error: %d (%s)\n", error, s); }

static int buffer_render_x	= 0;

//--------------------------------------------------------------------------------------------------------------
// Vector-based waveform rendering
//

#define VECTOR_WAVE_WIDTH		1024
#define VECTOR_WAVE_X			0
#define VECTOR_WAVE_Y			256
#define VECTOR_WAVE_HALFHEIGHT	256
#define VECTOR_WAVE_SCALE		129		// roughly 32767 divided by 256 with a safety margin

#define CALC_Y_COORD(sample)	(VECTOR_WAVE_Y + (sample / VECTOR_WAVE_SCALE))

static int16_t last_sample = 0;
static VGPath wave_path;
static VGPaint wave_stroke_paint;
static VGubyte first_sample_segment_commands[1] = { VG_MOVE_TO_ABS };
static VGubyte horiz_segment_commands[3] = { VG_MOVE_TO_ABS, VG_VLINE_TO_ABS, VG_HLINE_TO_ABS };
static VGubyte sample_segment_commands[128];
static VGshort sample_segment_coords[128 * 2];

static void vector_wave_event_handler(gfx_event_t *event)
{
	size_t sample_count = event->size / BYTES_PER_SAMPLE;
	int16_t *sample_data = (int16_t*) event->ptr;

	if (buffer_render_x < VECTOR_WAVE_WIDTH)
	{
		if (buffer_render_x == 0)
		{
			VGshort segment_coords[2];
			segment_coords[0] = VECTOR_WAVE_X;
			segment_coords[1] = CALC_Y_COORD(last_sample);

			vgAppendPathData(wave_path, 1, first_sample_segment_commands, segment_coords);
			VG_ERROR_CHECK("vgAppendPathData - wave 1");
		}

		VGshort *coord_ptr = sample_segment_coords;
		int16_t *sample_ptr = sample_data;
		size_t coord_count = sample_count;
		while (coord_count > 0 && buffer_render_x < VECTOR_WAVE_WIDTH)
		{
			*coord_ptr++ = VECTOR_WAVE_X + buffer_render_x;
			*coord_ptr++ = CALC_Y_COORD(*sample_ptr);

			sample_ptr += CHANNELS_PER_SAMPLE;
			buffer_render_x++;
			coord_count--;
		}

		vgAppendPathData(wave_path, sample_count - coord_count, sample_segment_commands, sample_segment_coords);
		VG_ERROR_CHECK("vgAppendPathData - wave 2");
	}

	last_sample = sample_data[(sample_count - 1) * CHANNELS_PER_SAMPLE];
	free(event->ptr);
}

static void vector_silence_event_handler(gfx_event_t *event)
{
	if (buffer_render_x < VECTOR_WAVE_WIDTH)
	{
		size_t sample_count = event->size / BYTES_PER_SAMPLE;

		VGshort segment_coords[4];
		segment_coords[0] = VECTOR_WAVE_X + buffer_render_x;
		segment_coords[1] = CALC_Y_COORD(last_sample);
		segment_coords[2] = CALC_Y_COORD(0);
        segment_coords[3] = VECTOR_WAVE_X + MIN(buffer_render_x + sample_count, VECTOR_WAVE_WIDTH);
		vgAppendPathData(wave_path, 3, horiz_segment_commands, segment_coords);
		VG_ERROR_CHECK("vgAppendPathData - silence");

        buffer_render_x += sample_count;
	}

	last_sample = 0;
}

static void vector_swap_event_handler(gfx_event_t *event)
{
	static VGfloat clear_colour[] = { 0.0f, 0.0f, 64.0f, 1.0f };

	vgSetfv(VG_CLEAR_COLOR, 4, clear_colour);
	vgClear(VECTOR_WAVE_X, VECTOR_WAVE_Y - VECTOR_WAVE_HALFHEIGHT, VECTOR_WAVE_WIDTH + 1, VECTOR_WAVE_HALFHEIGHT * 2);

	vgSetf(VG_STROKE_LINE_WIDTH, 1.0f);
	vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
	vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_MITER);
	vgSetPaint(wave_stroke_paint, VG_STROKE_PATH);
	vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);
	vgDrawPath(wave_path, VG_STROKE_PATH);
	VG_ERROR_CHECK("vgDrawPath");
	vgClearPath(wave_path, VG_PATH_CAPABILITY_APPEND_TO);
	VG_ERROR_CHECK("vgClearPath");
	buffer_render_x = 0;
}

static void vector_post_init_handler(gfx_event_t *event)
{
	wave_path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_S_16, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_APPEND_TO);
	VG_ERROR_CHECK("vgCreatePath");

	VGfloat line_colour[4] = { 0.0f, 255.0f, 0.0f, 255.0f };
	wave_stroke_paint = vgCreatePaint();
	vgSetParameteri(wave_stroke_paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(wave_stroke_paint, VG_PAINT_COLOR, 4, line_colour);
}

void gfx_wave_render_vector_initialise()
{
	gfx_register_event_handler(GFX_EVENT_POSTINIT, vector_post_init_handler);
	gfx_register_event_handler(GFX_EVENT_WAVE, vector_wave_event_handler);
	gfx_register_event_handler(GFX_EVENT_SILENCE, vector_silence_event_handler);
	gfx_register_event_handler(GFX_EVENT_BUFFERSWAP, vector_swap_event_handler);

	buffer_render_x = 0;
	last_sample = 0;

	memset(sample_segment_commands, VG_LINE_TO_ABS, sizeof(sample_segment_commands));
}

void gfx_wave_render_vector_deinitialise()
{
	vgDestroyPath(wave_path);
	vgDestroyPaint(wave_stroke_paint);
}

//--------------------------------------------------------------------------------------------------------------
// Entrypoint
//

void gfx_wave_render_initialise()
{
	gfx_wave_render_vector_initialise();
}

void gfx_wave_render_deinitialise()
{
	gfx_wave_render_vector_deinitialise();
}

