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
#define SYNTH_ENVELOPE_COUNT					3
#define SYNTH_GLOBAL_ENVELOPE_INSTANCE			0
#define SYNTH_GLOBAL_ENVELOPE_INSTANCE_COUNT	1
#define SYNTH_VOICE_ENVELOPE_INSTANCE_BASE		(SYNTH_GLOBAL_ENVELOPE_INSTANCE_COUNT)

extern const char*	SYNTH_MOD_SOURCE_LFO;

extern const char*	SYNTH_MOD_SINK_NOTE_AMPLITUDE;
extern const char*	SYNTH_MOD_SINK_NOTE_PITCH;

typedef struct synth_model_t synth_model_t;

typedef struct lfo_source_t
{
	mod_matrix_source_t	source;
	lfo_t				lfo;
} lfo_source_t;

typedef struct envelope_source_t
{
	mod_matrix_source_t		source;
	envelope_instance_t*	envelope_instance;
} envelope_source_t;

typedef struct synth_model_param_sink_t
{
	mod_matrix_sink_t	sink;
	synth_model_t*		synth_model;
} synth_model_param_sink_t;

struct synth_model_t
{
	// Settings
	setting_t*	setting_master_volume;
	setting_t*	setting_master_waveform;

	// Components
	envelope_t 				envelope[SYNTH_ENVELOPE_COUNT];
	filter_definition_t		global_filter_def;
	envelope_instance_t*	envelope_instances;
	lfo_t					lfo_def;
	int32_t* 				ducking_levels;

	// Sources
	lfo_source_t		lfo_source;
	envelope_source_t	envelope_source[SYNTH_ENVELOPE_COUNT];

	// Sinks
	synth_model_param_sink_t voice_amplitude_sink;
	synth_model_param_sink_t voice_pitch_sink;
	synth_model_param_sink_t voice_filter_q_sink;
	synth_model_param_sink_t voice_filter_freq_sink;
	synth_model_param_sink_t lfo_amplitude_sink;
	synth_model_param_sink_t lfo_freq_sink;

	// Voices
	int			voice_count;
	int			active_voices;
	int			ending_voices;
	int			global_envelopes_released;
	int			voice_amplitude_envelope_count;
	voice_t* 	voice;
};

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
	synth_model_t* synth_model;
	int32_t	timestep_ms;
	size_t	sample_count;
	void* buffer_data;
} synth_update_state_t;

extern void synth_model_initialise(synth_model_t* synth_model, int voice_count);
extern void synth_model_set_midi_channel(synth_model_t* synth_model, int midi_channel);
extern void synth_model_set_ducking_levels(synth_model_t* synth_model, int32_t* ducking_levels);
extern void synth_model_update(synth_model_t* synth_model, synth_update_state_t* update_state);
extern void synth_model_play_note(synth_model_t* synth_model, int channel, unsigned char midi_note);
extern void synth_model_stop_note(synth_model_t* synth_model, int channel, unsigned char midi_note);
extern void synth_model_deinitialise(synth_model_t* synth_model);

#endif /* SYNTH_MODEL_H_ */
