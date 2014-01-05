/*
 * envelope.c
 *
 *  Created on: 24 Nov 2012
 *      Author: ntuckett
 */

#include "envelope.h"
#include <stdlib.h>
#include "logging.h"
#include "fixed_point_math.h"

#define ENV_FIXED_PRECISION		12
#define MAX_ENVELOPE_CALLBACKS	4

typedef struct envelope_callback_info_t envelope_callback_info_t;
typedef struct envelope_callback_info_t
{
	envelope_callback_t		callback;
	void*					data;
	envelope_callback_info_t*	next_callback;
} envelope_callback_info_t;

static envelope_callback_info_t* envelope_callback_head = NULL;
static envelope_callback_info_t* envelope_callback_free = NULL;
static envelope_callback_info_t envelope_callback_info[MAX_ENVELOPE_CALLBACKS];

static int32_t calculate_level(envelope_instance_t *instance, envelope_stage_t* stage, int32_t elapsed_time_ms)
{
	int32_t start_level;

	if (stage->start_level == LEVEL_CURRENT)
	{
		start_level = instance->ref_level;
	}
	else
	{
		start_level = stage->start_level;
	}

	if (stage->duration > 0) {
		fixed_t level_scale = fixed_divide_at(elapsed_time_ms, stage->duration, ENV_FIXED_PRECISION);
		int32_t stage_delta = fixed_mul_at(stage->end_level - start_level, level_scale, ENV_FIXED_PRECISION);
		return start_level + stage_delta;
	}
	else
	{
		return stage->end_level;
	}
}

void envelopes_initialise()
{
	for (int i = 0; i < MAX_ENVELOPE_CALLBACKS; i++)
	{
		envelope_callback_info[i].next_callback = envelope_callback_free;
		envelope_callback_free = envelope_callback_info + i;
	}
}

void envelopes_add_callback(envelope_callback_t callback, void* callback_data)
{
	if (envelope_callback_free != NULL)
	{
		envelope_callback_info_t* callback_info = envelope_callback_free;
		envelope_callback_free = envelope_callback_free->next_callback;

		callback_info->callback 		= callback;
		callback_info->data 			= callback_data;
		callback_info->next_callback 	= envelope_callback_head;

		envelope_callback_head = callback_info;
	}
	else
	{
		LOG_ERROR("No voice callbacks available");
	}
}

void envelopes_remove_callback(envelope_callback_t callback)
{
	envelope_callback_info_t* callback_info = envelope_callback_head;
	envelope_callback_info_t** last_link = &envelope_callback_head;

	while (callback_info != NULL)
	{
		if (callback_info->callback == callback)
		{
			*last_link = callback_info->next_callback;
			callback_info->callback 		= NULL;
			callback_info->data				= NULL;
			callback_info->next_callback 	= envelope_callback_free;

			envelope_callback_free = callback_info;
			break;
		}
		else
		{
			last_link = &callback_info->next_callback;
			callback_info = callback_info->next_callback;
		}
	}
}

void envelope_make_callback(envelope_event_t event, envelope_instance_t* envelope)
{
	envelope_callback_info_t* callback_info = envelope_callback_head;

	while (callback_info != NULL)
	{
		callback_info->callback(event, envelope, callback_info->data);
		callback_info = callback_info->next_callback;
	}
}

void envelope_init(envelope_instance_t *instance, envelope_t* envelope)
{
	instance->stage		= ENVELOPE_STAGE_INACTIVE;
	instance->envelope	= envelope;
}

void envelope_start(envelope_instance_t *instance)
{
	instance->time_ms		= 0;
	instance->ref_level		= instance->envelope->stages[0].start_level;
	instance->last_level	= instance->ref_level;
	instance->stage			= 0;
	instance->ref_time_ms	= 0;
	envelope_make_callback(ENVELOPE_EVENT_STARTING, instance);
}

int32_t envelope_step(envelope_instance_t *instance, int32_t timestep_ms)
{
	int32_t level = instance->last_level;
	instance->time_ms += timestep_ms;

	if (instance->stage >= 0)
	{
		envelope_stage_t *stage = instance->envelope->stages + instance->stage;

		while (1) {
			int32_t	stage_time_elapsed = instance->time_ms - instance->ref_time_ms;
			if (stage->duration != DURATION_HELD && stage_time_elapsed > stage->duration)
			{
				instance->stage++;
				if (instance->stage == instance->envelope->stage_count)
				{
					level = instance->envelope->stages[instance->stage - 1].end_level;
					instance->stage = ENVELOPE_STAGE_INACTIVE;
					envelope_make_callback(ENVELOPE_EVENT_COMPLETED, instance);
					break;
				}
				else
				{
					level = calculate_level(instance, stage, stage->duration);
					instance->ref_time_ms += stage->duration;
					instance->ref_level = level;
					stage++;
					envelope_make_callback(ENVELOPE_EVENT_STAGE_CHANGE, instance);
				}
			}
			else
			{
				level = calculate_level(instance, stage, stage_time_elapsed);
				break;
			}
		}

		instance->last_level = level;
	}

	return level;
}

int envelope_completed(envelope_instance_t *instance)
{
	return instance->stage == ENVELOPE_STAGE_INACTIVE;
}

void envelope_go_to_stage(envelope_instance_t *instance, int32_t stage_id)
{
	instance->stage 		= stage_id;
	instance->ref_level 	= instance->last_level;
	instance->ref_time_ms	= instance->time_ms;
	envelope_make_callback(ENVELOPE_EVENT_STAGE_CHANGE, instance);
}
