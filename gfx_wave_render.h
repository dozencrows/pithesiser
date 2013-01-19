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
extern void gfx_wave_render_wavelength(wave_renderer_t *renderer, int32_t wavelength_samples);
extern void gfx_wave_renderer_destroy(wave_renderer_t *waveform_renderer);

#endif /* GFX_WAVE_RENDER_H_ */
