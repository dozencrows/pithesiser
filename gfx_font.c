/*
 * gfx_font.c
 *
 *  Created on: 8 Sep 2013
 *      Author: ntuckett
 *
 *  Derived from code on https://github.com/ajstarks/openvg
 *
 */

#include "gfx_font.h"
#include <string.h>
#include "VG/openvg.h"

static const int MAXFONTPATH = 256;

font_info_t gfx_load_font(
				const int *points,
				const int *point_indices,
				const unsigned char *instructions_set,
				const int *instruction_indices,
				const int *instruction_counts,
				const int *adv,
				const short *cmap,
				int ng
				)
{
	font_info_t f;
	int i;

	memset(f.glyphs, 0, MAXFONTPATH * sizeof(VGPath));
	if (ng > MAXFONTPATH)
	{
		return f;
	}
	for (i = 0; i < ng; i++)
	{
		const int *p = &points[point_indices[i] * 2];
		const unsigned char *instructions = &instructions_set[instruction_indices[i]];
		int ic = instruction_counts[i];
		VGPath path = vgCreatePath(
						VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_S_32,
						1.0f / 65536.0f, 0.0f, 0, 0,
						VG_PATH_CAPABILITY_ALL
						);
		f.glyphs[i] = path;
		if (ic)
		{
			vgAppendPathData(path, ic, instructions, p);
		}
	}
	f.character_map = cmap;
	f.glyph_advances = adv;
	f.count = ng;
	return f;
}

void gfx_unload_font(font_info_t* f)
{
	VGPath * glyphs = f->glyphs;
	int i;
	for (i = 0; i < f->count; i++)
	{
		vgDestroyPath(glyphs[i]);
	}
}

void gfx_render_text(float x, float y, const char *s, const font_info_t* f, int point_size)
{
	VGfloat size = (VGfloat) point_size, xx = x, mm[9];
	int i;
	const VGfloat scaled_size = size / 65536.0f;

	vgGetMatrix(mm);
	VGfloat render_mat[9] = {
		size, 0.0f, 0.0f,
		0.0f, size, 0.0f,
		xx, y, 1.0f
	};
	vgMultMatrix(render_mat);
	vgGetMatrix(render_mat);

	for (i = 0; i < (int)strlen(s); i++)
	{
		unsigned int character = (unsigned int)s[i];
		int glyph = f->character_map[character];
		if (glyph > -1)
		{
			vgLoadMatrix(render_mat);
			vgDrawPath(f->glyphs[glyph], VG_FILL_PATH);
			render_mat[6] += scaled_size * f->glyph_advances[glyph];
		}
	}
	vgLoadMatrix(mm);
}

float gfx_text_width(const char *s, const font_info_t* f, int point_size)
{
	int i;
	VGfloat tw = 0.0;
	VGfloat size = (VGfloat) point_size;
	const VGfloat scaled_size = size / 65536.0f;

	for (i = 0; i < (int)strlen(s); i++)
	{
		unsigned int character = (unsigned int)s[i];
		int glyph = f->character_map[character];
		if (glyph > -1)
		{
			tw += scaled_size * f->glyph_advances[glyph];
		}
	}

	return tw;
}
