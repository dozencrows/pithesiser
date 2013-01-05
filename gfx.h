/*
 * gfx.h
 *
 *  Created on: 2 Dec 2012
 *      Author: ntuckett
 */

#ifndef GFX_H_
#define GFX_H_

#include <stddef.h>

#define SYSTEM_FRAME_RATE	60

extern void gfx_initialise();
extern void gfx_deinitialise();
extern void gfx_set_frame_complete_threshold(size_t complete_threshold);
extern void gfx_advance_frame_progress(size_t progress);

#endif /* GFX_H_ */
