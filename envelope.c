/*
 * envelope.c
 *
 *  Created on: 24 Nov 2012
 *      Author: ntuckett
 */

#include "envelope.h"
#include "fixed_point_math.h"

#define ENV_FIXED_PRECISION		12

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

	int32_t stage_delta = 0;
	if (stage->duration > 0) {
		fixed_t level_scale = fixed_divide_at(elapsed_time_ms, stage->duration, ENV_FIXED_PRECISION);
		stage_delta = fixed_mul_at(stage->end_level - start_level, level_scale, ENV_FIXED_PRECISION);
	}

	return start_level + stage_delta;
}

void envelope_init(envelope_instance_t *instance, envelope_t* envelope)
{
	instance->stage		= ENVELOPE_STAGE_INACTIVE;
	instance->envelope	= envelope;
}

void envelope_start(envelope_instance_t *instance)
{
	instance->time_ms		= 0;
	instance->ref_level		= 0;
	instance->last_level	= 0;
	instance->stage			= 0;
	instance->ref_time_ms	= 0;
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
					instance->stage = ENVELOPE_STAGE_INACTIVE;
					level = 0;
					break;
				}
				else
				{
					level = calculate_level(instance, stage, stage->duration);
					instance->ref_time_ms += stage->duration;
					instance->ref_level = level;
					stage++;
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
}
