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
#include "VG/openvg.h"
#include "gfx.h"
#include "gfx_event.h"
#include "gfx_event_types.h"
#include "system_constants.h"

typedef struct wave_renderer_state_t
{
	VGPaint line_paint;
	int half_height;
	int max_rendered_samples;
	int rendered_samples;
	VGPath path[2];
	int16_t render_path;
	int16_t draw_path;
	int16_t last_sample;
} wave_renderer_state_t;

typedef struct waveform_renderer_internal_t
{
	wave_renderer_t	definition;
	wave_renderer_state_t state;
} wave_renderer_internal_t;

#define BYTES_PER_CHANNEL	(sizeof(int16_t))
#define CHANNELS_PER_SAMPLE	2
#define BYTES_PER_SAMPLE	(BYTES_PER_CHANNEL * CHANNELS_PER_SAMPLE)

#define VG_ERROR_CHECK(s)	{ VGErrorCode error = vgGetError(); if (error != VG_NO_ERROR) printf("VG Error: %d (%s)\n", error, s); }

#define CALC_Y_COORD(renderer, sample)	(renderer->definition.y + (sample / renderer->definition.amplitude_scale))

static VGubyte first_sample_segment_commands[1] = { VG_MOVE_TO_ABS };
static VGubyte horiz_segment_commands[3] = { VG_MOVE_TO_ABS, VG_VLINE_TO_ABS, VG_HLINE_TO_ABS };
static VGubyte sample_segment_commands[128];
static VGshort sample_segment_coords[128 * 2];

//--------------------------------------------------------------------------------------------------------------
// Internal functionality
//
static void initialise_renderer(wave_renderer_internal_t *renderer)
{
	renderer->state.line_paint = vgCreatePaint();
	VG_ERROR_CHECK("vgCreatePaint");
	vgSetParameteri(renderer->state.line_paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(renderer->state.line_paint, VG_PAINT_COLOR, 4, renderer->definition.line_colour);
	renderer->state.half_height = renderer->definition.height / 2;
	renderer->state.max_rendered_samples = renderer->definition.width;
	renderer->state.path[0] = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_S_16, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_APPEND_TO);
	VG_ERROR_CHECK("vgCreatePath 0");
	renderer->state.path[1] = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_S_16, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_APPEND_TO);
	VG_ERROR_CHECK("vgCreatePath 1");
	renderer->state.render_path = 0;
}

static void deinitialise_renderer(wave_renderer_internal_t *renderer)
{
	vgDestroyPath(renderer->state.path[0]);
	vgDestroyPath(renderer->state.path[1]);
	vgDestroyPaint(renderer->state.line_paint);
}

static size_t render_samples_to_path(wave_renderer_internal_t *renderer, size_t sample_count, int16_t *sample_data, VGPath path)
{
	size_t rendered_samples = 0;

	if (renderer->state.rendered_samples == 0)
	{
		VGshort segment_coords[2];
		segment_coords[0] = renderer->definition.x;
		segment_coords[1] = CALC_Y_COORD(renderer, renderer->state.last_sample);

		vgAppendPathData(path, 1, first_sample_segment_commands, segment_coords);
		VG_ERROR_CHECK("vgAppendPathData - wave 1");
	}

	if (renderer->state.rendered_samples < renderer->state.max_rendered_samples)
	{
		VGshort *coord_ptr = sample_segment_coords;
		int16_t *sample_ptr = sample_data;
		size_t coord_count = sample_count;
		while (coord_count > 0 && renderer->state.rendered_samples < renderer->state.max_rendered_samples)
		{
			*coord_ptr++ = renderer->definition.x + renderer->state.rendered_samples;
			*coord_ptr++ = CALC_Y_COORD(renderer, *sample_ptr);

			sample_ptr += CHANNELS_PER_SAMPLE;
			renderer->state.rendered_samples++;
			coord_count--;
		}
		rendered_samples = sample_count - coord_count;
		vgAppendPathData(path, rendered_samples, sample_segment_commands, sample_segment_coords);
		VG_ERROR_CHECK("vgAppendPathData - wave 2");
		renderer->state.last_sample = *(sample_ptr - CHANNELS_PER_SAMPLE);
	}

	return rendered_samples;
}

static size_t render_silence_to_path(wave_renderer_internal_t *renderer, size_t sample_count, VGPath path)
{
	size_t rendered_samples = 0;

	if (renderer->state.rendered_samples < renderer->state.max_rendered_samples)
	{
		VGshort segment_coords[4];
		rendered_samples = MIN(sample_count, renderer->state.max_rendered_samples - renderer->state.rendered_samples);
		segment_coords[0] = renderer->definition.x + renderer->state.rendered_samples;
		segment_coords[1] = CALC_Y_COORD(renderer, renderer->state.last_sample);
		segment_coords[2] = CALC_Y_COORD(renderer, 0);
        segment_coords[3] = renderer->definition.x + renderer->state.rendered_samples + rendered_samples;
		vgAppendPathData(path, 3, horiz_segment_commands, segment_coords);
		VG_ERROR_CHECK("vgAppendPathData - silence");

		renderer->state.rendered_samples += rendered_samples;
		renderer->state.last_sample = 0;
	}

	return rendered_samples;
}

static size_t render_to_path(wave_renderer_internal_t *renderer, size_t sample_count, int16_t *sample_data, size_t sample_offset, VGPath path)
{
	if (sample_data != NULL)
	{
		return(render_samples_to_path(renderer, sample_count, sample_data + (sample_offset * CHANNELS_PER_SAMPLE), path));
	}
	else
	{
		return(render_silence_to_path(renderer, sample_count, path));
	}
}

static void render_waveform_data(wave_renderer_internal_t *renderer, size_t sample_count, int16_t *sample_data)
{
	VGPath path = renderer->state.path[renderer->state.render_path];
	size_t rendered_samples = render_to_path(renderer, sample_count, sample_data, 0, path);

	if (renderer->state.rendered_samples == renderer->state.max_rendered_samples)
	{
		gfx_set_frame_progress(gfx_get_frame_complete_threshold());
		renderer->state.draw_path = renderer->state.render_path;
		renderer->state.render_path ^= 1;
		renderer->state.rendered_samples = 0;

		if (rendered_samples < sample_count)
		{
			size_t samples_for_next_frame = sample_count - rendered_samples;
			path = renderer->state.path[renderer->state.render_path];
			render_to_path(renderer, samples_for_next_frame, sample_data, rendered_samples, path);
		}
	}
}

static void update_display(wave_renderer_internal_t *renderer)
{
	VGPath path = renderer->state.path[renderer->state.draw_path];
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgSetfv(VG_CLEAR_COLOR, 4, renderer->definition.background_colour);
	vgClear(renderer->definition.x, renderer->definition.y - renderer->state.half_height,
			renderer->definition.width + 1, renderer->definition.height);

	vgSetf(VG_STROKE_LINE_WIDTH, renderer->definition.line_width);
	vgSeti(VG_STROKE_CAP_STYLE, renderer->definition.line_cap_style);
	vgSeti(VG_STROKE_JOIN_STYLE, renderer->definition.line_join_style);
	vgSetPaint(renderer->state.line_paint, VG_STROKE_PATH);
	vgSeti(VG_RENDERING_QUALITY, renderer->definition.rendering_quality);
	vgDrawPath(path, VG_STROKE_PATH);
	VG_ERROR_CHECK("vgDrawPath");
	vgClearPath(path, VG_PATH_CAPABILITY_APPEND_TO);
	VG_ERROR_CHECK("vgClearPath");
}

//--------------------------------------------------------------------------------------------------------------
// Event handlers
//
static void vector_wave_event_handler(gfx_event_t *event, gfx_object_t *receiver)
{
	render_waveform_data((wave_renderer_internal_t*)receiver, event->size / BYTES_PER_SAMPLE, (int16_t*)event->ptr);
	free(event->ptr);
}

static void vector_silence_event_handler(gfx_event_t *event, gfx_object_t *receiver)
{
	render_waveform_data((wave_renderer_internal_t*)receiver, event->size / BYTES_PER_SAMPLE, NULL);
}

static void vector_swap_event_handler(gfx_event_t *event, gfx_object_t *receiver)
{
	update_display((wave_renderer_internal_t*)receiver);
}

static void vector_post_init_handler(gfx_event_t *event, gfx_object_t *receiver)
{
	initialise_renderer((wave_renderer_internal_t*)receiver);
	gfx_set_frame_complete_threshold(SYSTEM_SAMPLE_RATE / SYSTEM_FRAME_RATE);
}

//--------------------------------------------------------------------------------------------------------------
// Public interface
//

//
// Module initialisation
//
void gfx_wave_render_initialise()
{
	memset(sample_segment_commands, VG_LINE_TO_ABS, sizeof(sample_segment_commands));
}

void gfx_wave_render_deinitialise()
{
}

//
// Instance functions
//
wave_renderer_t *gfx_wave_renderer_create(object_id_t id)
{
	wave_renderer_internal_t *renderer = (wave_renderer_internal_t*) calloc(1, sizeof(wave_renderer_internal_t));

	gfx_object_t *gfx_object = &renderer->definition.gfx_object;
	gfx_object->id = id;
	gfx_register_event_receiver_handler(GFX_EVENT_POSTINIT, vector_post_init_handler, gfx_object);
	gfx_register_event_receiver_handler(GFX_EVENT_WAVE, vector_wave_event_handler, gfx_object);
	gfx_register_event_receiver_handler(GFX_EVENT_SILENCE, vector_silence_event_handler, gfx_object);
	gfx_register_event_receiver_handler(GFX_EVENT_BUFFERSWAP, vector_swap_event_handler, gfx_object);

	renderer->definition.line_width 		= 1.0f;
	renderer->definition.line_cap_style 	= VG_CAP_BUTT;
	renderer->definition.line_join_style	= VG_JOIN_MITER;
	renderer->definition.rendering_quality	= VG_RENDERING_QUALITY_NONANTIALIASED;

	return (wave_renderer_t*)renderer;
}

void gfx_wave_render_wavelength(wave_renderer_t *renderer, int32_t wavelength_samples_fx)
{
	wave_renderer_internal_t *renderer_int = (wave_renderer_internal_t*)renderer;

	if (wavelength_samples_fx <= 0) {
		renderer_int->definition.tuned_wavelength_fx = 0;
		renderer_int->state.max_rendered_samples = renderer_int->definition.width;
	}
	else {
		renderer_int->definition.tuned_wavelength_fx = wavelength_samples_fx;

		int64_t wave_max_samples_fx = renderer_int->definition.width << FIXED_PRECISION;

		if (wavelength_samples_fx < wave_max_samples_fx)
		{
			// Wavelength count is result of a truncation so rendering does not overflow maximum area
			int64_t wavelengths_count = (((wave_max_samples_fx << FIXED_PRECISION) / (int64_t)wavelength_samples_fx)) >> FIXED_PRECISION;
			int samples_count = ((wavelengths_count * (int64_t)wavelength_samples_fx) + FIXED_HALF) >> FIXED_PRECISION;
			renderer_int->state.max_rendered_samples = samples_count;
		}
		else
		{
			renderer_int->state.max_rendered_samples = renderer_int->definition.width;
		}
	}
}

void gfx_wave_renderer_destroy(wave_renderer_t *waveform_renderer)
{
	deinitialise_renderer((wave_renderer_internal_t*) waveform_renderer);
	free(waveform_renderer);
}
