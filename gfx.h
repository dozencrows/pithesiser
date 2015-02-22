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
 * gfx.h
 *
 *  Created on: 2 Dec 2012
 *      Author: ntuckett
 */

#ifndef GFX_H_
#define GFX_H_

#include <stddef.h>

extern void gfx_initialise();
extern void gfx_deinitialise();
extern void gfx_get_screen_resolution(int *width, int *height);
extern void gfx_set_frame_complete_threshold(size_t complete_threshold);
extern size_t gfx_get_frame_complete_threshold();
extern void gfx_advance_frame_progress(size_t progress_delta);
extern void gfx_set_frame_progress(size_t progress);
extern void gfx_screenshot(const char* screenshot_file_path);

#endif /* GFX_H_ */
