/*
 * gfx_event.c
 *
 *  Created on: 9 Dec 2012
 *      Author: ntuckett
 */

#include "gfx_event.h"
#include <pthread.h>
#include <string.h>
#include <semaphore.h>

#define GFX_MAX_EVENT_HANDLERS	64
#define GFX_EVENT_BUFFER_SIZE	256
#define GFX_EVENT_BUFFER_MASK	(GFX_EVENT_BUFFER_SIZE - 1)

typedef struct gfx_event_handler_record_t
{
	gfx_event_type_t 	type;
	gfx_event_handler_t handler;
} gfx_event_handler_record_t;

typedef struct gfx_event_receiver_handler_record_t
{
	gfx_event_type_t 	type;
	gfx_object_t		*receiver;
	gfx_event_handler_t handler;
} gfx_event_receiver_handler_record_t;

gfx_event_handler_record_t	gfx_event_handlers[GFX_MAX_EVENT_HANDLERS];
int gfx_event_handler_count = 0;

gfx_event_receiver_handler_record_t gfx_event_receiver_handlers[GFX_MAX_EVENT_HANDLERS];
int gfx_event_receiver_handler_count = 0;

pthread_mutex_t	gfx_event_lock = PTHREAD_MUTEX_INITIALIZER;
sem_t gfx_event_semaphore;

gfx_event_t gfx_event_buffer[GFX_EVENT_BUFFER_SIZE];
int gfx_event_buffer_write_index = 0;
int gfx_event_buffer_read_index = 0;

void gfx_send_event(gfx_event_t *event)
{
	pthread_mutex_lock(&gfx_event_lock);

	int next_write_index = (gfx_event_buffer_write_index + 1) & GFX_EVENT_BUFFER_MASK;
	if (next_write_index != gfx_event_buffer_read_index)
	{
		memcpy(&gfx_event_buffer[gfx_event_buffer_write_index], event, sizeof(gfx_event_t));
		gfx_event_buffer_write_index = next_write_index;
	}

	pthread_mutex_unlock(&gfx_event_lock);
	sem_post(&gfx_event_semaphore);
}

gfx_event_t *gfx_pop_event(gfx_event_t *event)
{
	gfx_event_t *return_event = NULL;

	pthread_mutex_lock(&gfx_event_lock);

	if (gfx_event_buffer_read_index != gfx_event_buffer_write_index) {
		memcpy(event, &gfx_event_buffer[gfx_event_buffer_read_index], sizeof(gfx_event_t));
		return_event = event;
		gfx_event_buffer_read_index = (gfx_event_buffer_read_index + 1) & GFX_EVENT_BUFFER_MASK;
	}
	pthread_mutex_unlock(&gfx_event_lock);

	return return_event;
}

int gfx_get_event_count()
{
	int event_count;

	pthread_mutex_lock(&gfx_event_lock);

	if (gfx_event_buffer_read_index <= gfx_event_buffer_write_index)
	{
		event_count = gfx_event_buffer_write_index - gfx_event_buffer_read_index;
	}
	else
	{
		event_count = GFX_EVENT_BUFFER_SIZE - gfx_event_buffer_read_index + gfx_event_buffer_write_index;
	}

	pthread_mutex_unlock(&gfx_event_lock);

	return event_count;
}

void gfx_register_event_global_handler(gfx_event_type_t event_type, gfx_event_handler_t handler)
{
	if (gfx_event_handler_count < GFX_MAX_EVENT_HANDLERS)
	{
		gfx_event_handlers[gfx_event_handler_count].type = event_type;
		gfx_event_handlers[gfx_event_handler_count].handler = handler;
		gfx_event_handler_count++;
	}
}

void gfx_register_event_receiver_handler(gfx_event_type_t event_type, gfx_event_handler_t handler, gfx_object_t *receiver)
{
	if (gfx_event_receiver_handler_count < GFX_MAX_EVENT_HANDLERS)
	{
		gfx_event_receiver_handlers[gfx_event_receiver_handler_count].type = event_type;
		gfx_event_receiver_handlers[gfx_event_receiver_handler_count].receiver = receiver;
		gfx_event_receiver_handlers[gfx_event_receiver_handler_count].handler = handler;
		gfx_event_receiver_handler_count++;
	}
}

void gfx_process_event(gfx_event_t *event)
{
	if (event->receiver_id == GFX_ANY_OBJECT)
	{
		gfx_event_handler_record_t *handler_record = gfx_event_handlers;
		for (int i = 0; i < gfx_event_handler_count; i++, handler_record++)
		{
			if (handler_record->type == event->type)
			{
				handler_record->handler(event, NULL);
			}
		}

		gfx_event_receiver_handler_record_t *receiver_handler_record = gfx_event_receiver_handlers;
		for (int i = 0; i < gfx_event_receiver_handler_count; i++, receiver_handler_record++)
		{
			if (receiver_handler_record->type == event->type)
			{
				receiver_handler_record->handler(event, receiver_handler_record->receiver);
			}
		}
	}
	else
	{
		gfx_event_receiver_handler_record_t *handler_record = gfx_event_receiver_handlers;
		for (int i = 0; i < gfx_event_receiver_handler_count; i++, handler_record++)
		{
			if (handler_record->type == event->type && handler_record->receiver->id == event->receiver_id)
			{
				handler_record->handler(event, handler_record->receiver);
			}
		}
	}
}

void gfx_event_initialise()
{
	sem_init(&gfx_event_semaphore, 0, 0);
}

void gfx_wait_for_event()
{
	sem_wait(&gfx_event_semaphore);
}
