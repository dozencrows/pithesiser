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
