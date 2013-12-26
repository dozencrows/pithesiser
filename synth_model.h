/*
 * synth_model.h
 *
 *  Created on: 26 Dec 2013
 *      Author: ntuckett
 */

#ifndef SYNTH_MODEL_H_
#define SYNTH_MODEL_H_

#include "envelope.h"
#include "waveform.h"
#include "filter.h"
#include "lfo.h"

// Forward declarations
typedef struct setting_t setting_t;

// Types and values
typedef struct synth_model_t
{
	envelope_t 			envelope[3];
	lfo_t 				lfo;
	filter_definition_t	global_filter_def;

	setting_t*	setting_master_volume;
	setting_t*	setting_master_waveform;
} synth_model_t;


#define STATE_UNCHANGED	0
#define STATE_UPDATED	1

typedef struct synth_state_t
{
	unsigned char	volume;
	unsigned char	waveform;
	unsigned char	envelope;
	unsigned char	lfo;
	unsigned char	filter;
} synth_state_t;

#endif /* SYNTH_MODEL_H_ */
