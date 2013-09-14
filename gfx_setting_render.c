/*
 * gfx_setting_renderer.c
 *
 *  Created on: 8 Sep 2013
 *      Author: ntuckett
 */

#include "gfx_setting_render.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gfx_event.h"
#include "gfx_event_types.h"
#include "gfx_font.h"
#include "gfx_font_resources.h"

typedef struct setting_renderer_state_t
{
	VGPaint text_paint;
	int display_update;
} setting_renderer_state_t;

typedef struct setting_render_internal_t
{
	setting_renderer_t	definition;
	setting_renderer_state_t state;
} setting_render_internal_t;

static void initialise_renderer(setting_render_internal_t* renderer)
{
	renderer->state.display_update = 1;
	renderer->state.text_paint = vgCreatePaint();
	vgSetParameteri(renderer->state.text_paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(renderer->state.text_paint, VG_PAINT_COLOR, 4, renderer->definition.text_colour);
}

static void deinitialise_renderer(setting_render_internal_t* renderer)
{
	vgDestroyPaint(renderer->state.text_paint);
}

static void update_display(setting_render_internal_t* renderer)
{
	// Render it out
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgSetfv(VG_CLEAR_COLOR, 4, renderer->definition.background_colour);
	vgClear(renderer->definition.x, renderer->definition.y,
			renderer->definition.width, renderer->definition.height);
	vgSetiv(VG_SCISSOR_RECTS, 4, &renderer->definition.x);
	vgSeti(VG_SCISSORING, VG_TRUE);

	vgTranslate(renderer->definition.x, renderer->definition.y);
	vgSeti(VG_RENDERING_QUALITY, renderer->definition.text_quality);
	vgSetPaint(renderer->state.text_paint, VG_FILL_PATH);
	gfx_render_text(renderer->definition.text_x_offset, renderer->definition.text_y_offset, renderer->definition.text, &gfx_font_sans, renderer->definition.text_size);
	VGfloat text_width = gfx_text_width(renderer->definition.text, &gfx_font_sans, renderer->definition.text_size);

	const size_t buffer_size = 512;
	char setting_text_buffer[buffer_size];
	setting_t* setting = renderer->definition.setting;

	switch(setting->type)
	{
		case SETTING_TYPE_INT:
		{
			snprintf(setting_text_buffer, buffer_size - 1, renderer->definition.format, setting_get_value_int(setting));
			break;
		}
		case SETTING_TYPE_FLOAT:
		{
			snprintf(setting_text_buffer, buffer_size - 1, renderer->definition.format, setting_get_value_float(setting));
			break;
		}
		case SETTING_TYPE_ENUM:
		{
			snprintf(setting_text_buffer, buffer_size - 1, renderer->definition.format, setting_get_value_enum(setting));
			break;
		}
		default:
			snprintf(setting_text_buffer, buffer_size - 1, "ILLEGAL SETTING TYPE: %d", (int)setting->type);
			break;
	}

	setting_text_buffer[buffer_size - 1] = 0;
	gfx_render_text(text_width + 2 + renderer->definition.text_x_offset, renderer->definition.text_y_offset,
						setting_text_buffer, &gfx_font_sans, renderer->definition.text_size);

	vgSeti(VG_SCISSORING, VG_FALSE);
}

//--------------------------------------------------------------------------------------------------------------
// Event handlers
//

static void refresh_handler(gfx_event_t* event, gfx_object_t* receiver)
{
	setting_render_internal_t* renderer = (setting_render_internal_t*)receiver;
	renderer->state.display_update = 1;
}

static void swap_handler(gfx_event_t* event, gfx_object_t* receiver)
{
	setting_render_internal_t* renderer = (setting_render_internal_t*)receiver;
	if (renderer->state.display_update)
	{
		update_display(renderer);
		renderer->state.display_update = 0;
	}
}

static void post_init_handler(gfx_event_t* event, gfx_object_t* receiver)
{
	initialise_renderer((setting_render_internal_t*)receiver);
}

//--------------------------------------------------------------------------------------------------------------
// Public interface
//

void gfx_setting_render_initialise()
{
}

void gfx_setting_render_deinitialise()
{
}

setting_renderer_t* gfx_setting_renderer_create(object_id_t id)
{
	setting_render_internal_t *renderer = (setting_render_internal_t*) calloc(1, sizeof(setting_render_internal_t));

	gfx_object_t *gfx_object = &renderer->definition.gfx_object;
	gfx_object->id = id;
	gfx_register_event_receiver_handler(GFX_EVENT_POSTINIT, post_init_handler, gfx_object);
	gfx_register_event_receiver_handler(GFX_EVENT_REFRESH, refresh_handler, gfx_object);
	gfx_register_event_receiver_handler(GFX_EVENT_BUFFERSWAP, swap_handler, gfx_object);

	renderer->definition.text_quality = VG_RENDERING_QUALITY_FASTER;

	return (setting_renderer_t*)renderer;
}

void gfx_setting_renderer_destroy(setting_renderer_t* renderer)
{
	deinitialise_renderer((setting_render_internal_t*)renderer);
	free(renderer);
}
