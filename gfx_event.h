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
