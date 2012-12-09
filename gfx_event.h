/*
 * gfx_event.h
 *
 *  Created on: 9 Dec 2012
 *      Author: ntuckett
 */

#ifndef GFX_EVENT_H_
#define GFX_EVENT_H_

typedef unsigned int gfx_event_type_t;

typedef struct gfx_event_t
{
	gfx_event_type_t type;
	union
	{
		void *ptr;
		int data;
	};
} gfx_event_t;

typedef void (*gfx_event_handler_t)(gfx_event_t *event);

extern void gfx_send_event(gfx_event_t *event);
extern int gfx_get_event_count();
extern gfx_event_t *gfx_pop_event(gfx_event_t *event);
extern void gfx_register_event_handler(gfx_event_type_t event_type, gfx_event_handler_t handler);
extern void gfx_handle_event(gfx_event_t *event);

#endif /* GFX_EVENT_H_ */
