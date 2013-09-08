/*
 * gfx_font.h
 *
 *  Created on: 8 Sep 2013
 *      Author: ntuckett
 */

#ifndef GFX_FONT_H_
#define GFX_FONT_H_

typedef struct font_info_t
{
	const short *character_map;
	const int *glyph_advances;
	int count;
	unsigned int glyphs[256];
} font_info_t;

extern font_info_t gfx_load_font(const int *points,
		  const int *point_indices,
		  const unsigned char *instructions_set,
		  const int *instruction_indices, const int *instruction_counts, const int *adv, const short *cmap, int ng);

extern void gfx_unload_font(font_info_t* f);
extern void gfx_render_text(float x, float y, const char *s, const font_info_t* f, int point_size);
extern float gfx_text_width(const char *s, const font_info_t* f, int point_size);

#endif /* GFX_FONT_H_ */
