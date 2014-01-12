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
#include "modulation_matrix.h"

// Forward declarations
typedef struct setting_t setting_t;
typedef struct voice_t voice_t;

// Types and values
#define SYNTH_ENVELOPE_COUNT			3
#define SYNTH_ENVELOPE_INSTANCE_COUNT

extern const char*	SYNTH_MOD_SOURCE_LFO;

extern const char*	SYNTH_MOD_SINK_NOTE_AMPLITUDE;
extern const char*	SYNTH_MOD_SINK_NOTE_PITCH;

typedef struct lfo_source_t
{
	mod_matrix_source_t	source;
	lfo_t				lfo;
} lfo_source_t;

typedef struct synth_model_t
{
	// Settings
	setting_t*	setting_master_volume;
	setting_t*	setting_master_waveform;

	// Components
	envelope_t 				envelope[SYNTH_ENVELOPE_COUNT];
	filter_definition_t		global_filter_def;
	envelope_instance_t*	envelope_instance[SYNTH_ENVELOPE_COUNT];

	// Sources
	lfo_source_t	lfo_source;

	// Sinks

	// Voices
	int			voice_count;
	int			active_voices;
	voice_t* 	voice;
} synth_model_t;

#define STATE_UNCHANGED	0
#define STATE_UPDATED	1

typedef struct synth_state_t
{
	unsigned char	volume;
	unsigned char	waveform;
	unsigned char	envelope[SYNTH_ENVELOPE_COUNT];
	unsigned char	lfo_params;
	unsigned char	filter;
} synth_state_t;

typedef struct synth_update_state_t
{
	int32_t	timestep_ms;
	size_t	sample_count;
} synth_update_state_t;

extern void synth_model_initialise(synth_model_t* synth_model, int voice_count);
extern void synth_model_deinitialise(synth_model_t* synth_model);
extern void synth_model_update(synth_model_t* synth_model, synth_update_state_t* update_state);

#endif /* SYNTH_MODEL_H_ */
