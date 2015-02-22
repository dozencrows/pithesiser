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
 * envelope.h
 *
 *  Created on: 24 Nov 2012
 *      Author: ntuckett
 */

#ifndef ENVELOPE_H_
#define ENVELOPE_H_

#include <sys/types.h>

#define ENVELOPE_STAGE_INACTIVE	-1
#define	ENVELOPE_STAGE_ATTACK	0
#define	ENVELOPE_STAGE_DECAY	1
#define	ENVELOPE_STAGE_SUSTAIN	2
#define	ENVELOPE_STAGE_RELEASE	3
#define ENVELOPE_STAGES_MAX		4

#define DURATION_HELD			-1
#define LEVEL_CURRENT			-1

typedef struct envelope_stage_t
{
	int32_t		start_level;
	int32_t		end_level;
	int32_t		duration;
} envelope_stage_t;

typedef struct envelope_t
{
	int32_t				peak;
	int					stage_count;
	envelope_stage_t 	*stages;
} envelope_t;

typedef struct envelope_instance_t
{
	envelope_t	*envelope;
	int			stage;
	int32_t		time_ms;
	int32_t		ref_level;
	int32_t		last_level;
	int32_t		ref_time_ms;
} envelope_instance_t;

typedef enum envelope_event_t
{
	ENVELOPE_EVENT_STARTING,
	ENVELOPE_EVENT_STAGE_CHANGE,
	ENVELOPE_EVENT_COMPLETED
} envelope_event_t;

typedef void (*envelope_callback_t)(envelope_event_t callback_event, envelope_instance_t* envelope, void* callback_data);

extern void envelopes_initialise();
extern void envelopes_add_callback(envelope_callback_t callback, void* callback_data);
extern void envelopes_remove_callback(envelope_callback_t callback);

extern void envelope_init(envelope_instance_t *instance, envelope_t* envelope);
extern void envelope_start(envelope_instance_t *instance);
extern int32_t envelope_step(envelope_instance_t *instance, int32_t timestep_ms);
extern int envelope_completed(envelope_instance_t *instance);
extern void envelope_go_to_stage(envelope_instance_t *instance, int32_t stage_id);

#endif /* ENVELOPE_H_ */

