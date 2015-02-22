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
 * gfx_event.h
 *
 *  Created on: 9 Dec 2012
 *      Author: ntuckett
 */

#ifndef GFX_EVENT_H_
#define GFX_EVENT_H_

#include <stddef.h>
#include "gfx_object.h"

#define GFX_EVENT_FLAG_OWNPTR	1		// indicates event owns dynamic memory at ptr, should free after processing

typedef unsigned int gfx_event_type_t;

typedef struct gfx_event_t
{
	gfx_event_type_t 	type;
	object_id_t			receiver_id;
	size_t			 	size;
	int					flags;
	union
	{
		void *ptr;
		int data;
	};
} gfx_event_t;

typedef void (*gfx_event_handler_t)(gfx_event_t *event, gfx_object_t *receiver);

extern void gfx_event_initialise();
extern void gfx_send_event(gfx_event_t *event);
extern void gfx_wait_for_event();
extern int gfx_get_event_count();
extern gfx_event_t *gfx_pop_event(gfx_event_t *event);
extern void gfx_register_event_global_handler(gfx_event_type_t event_type, gfx_event_handler_t handler);
extern void gfx_register_event_receiver_handler(gfx_event_type_t event_type, gfx_event_handler_t handler, gfx_object_t *receiver);
extern void gfx_process_event(gfx_event_t *event);

#endif /* GFX_EVENT_H_ */
