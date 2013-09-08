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
