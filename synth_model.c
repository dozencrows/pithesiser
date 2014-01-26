/*
 * synth_model.c
 *
 *  Created on: 31 Dec 2013
 *      Author: ntuckett
 */

#include "synth_model.h"
#include <stdlib.h>
#include <math.h>
#include "logging.h"
#include "fixed_point_math.h"
#include "voice.h"
#include "lfo.h"
#include "setting.h"

const char*	SYNTH_MOD_SOURCE_LFO			= "lfo";
const char*	SYNTH_MOD_SOURCE_ENVELOPE_1		= "envelope-1";
const char*	SYNTH_MOD_SOURCE_ENVELOPE_2		= "envelope-2";
const char*	SYNTH_MOD_SOURCE_ENVELOPE_3		= "envelope-3";

const char*	SYNTH_MOD_SINK_NOTE_AMPLITUDE	= "note-amplitude";
const char*	SYNTH_MOD_SINK_NOTE_PITCH		= "note-pitch";
const char*	SYNTH_MOD_SINK_FILTER_Q			= "filter-q";
const char*	SYNTH_MOD_SINK_FILTER_FREQ		= "filter-freq";
const char*	SYNTH_MOD_SINK_LFO_AMPLITUDE	= "lfo-amplitude";
const char*	SYNTH_MOD_SINK_LFO_FREQ			= "lfo-freq";

//=========================================================================================================================
// Internal synth model function forward declarations
//

extern void synth_model_start_global_envelopes(synth_model_t* synth_model);
extern void synth_model_start_voice_envelopes(synth_model_t* synth_model, int voice_index);
extern void synth_model_release_global_envelopes(synth_model_t* synth_model);
extern void synth_model_release_voice_envelopes(synth_model_t* synth_model, int voice_index);
extern void synth_model_init_envelopes(synth_model_t* synth_model, int voice_count);

//=========================================================================================================================
// Model parameter management
//
static void synth_model_init_param_sink(const char* name, base_update_t base_update, model_update_t model_update, synth_model_t* synth_model, synth_model_param_sink_t* sink)
{
	mod_matrix_init_sink(name, base_update, model_update, &sink->sink);
	mod_matrix_add_sink(&sink->sink);
	sink->synth_model = synth_model;
}

//=========================================================================================================================
// Modulation modelling
//

//-------------------------------------------------------------------------------------------------------------------------
// LFO modulation
//
// Note that as the LFO is both a source and two sinks, modulation changes applied to its parameters will lag
// one audio chunk before being used in the LFO update.
//
static void lfo_generate_value(mod_matrix_source_t* source, void* data)
{
	lfo_source_t* lfo_source = (lfo_source_t*)source;
	synth_update_state_t* state = (synth_update_state_t*)data;

	lfo_update(&lfo_source->lfo, state->sample_count);
}

static mod_matrix_value_t lfo_get_value(mod_matrix_source_t* source, int subsource_id)
{
	lfo_source_t* lfo_source = (lfo_source_t*)source;
	return (mod_matrix_value_t) lfo_source->lfo.value;
}

static void lfo_amplitude_base_update(mod_matrix_sink_t* sink, void* data)
{
	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;
	param_sink->synth_model->lfo_source.lfo.oscillator.level = param_sink->synth_model->lfo_def.oscillator.level;
}

static void lfo_amplitude_model_update(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;

	mod_matrix_value_t source_value = source->get_value(source, MOD_MATRIX_SINGLE_SOURCE);

	if (source_value > 0)
	{
		param_sink->synth_model->lfo_source.lfo.oscillator.level = source_value;
	}
	else
	{
		param_sink->synth_model->lfo_source.lfo.oscillator.level = 0;
	}
}

static void lfo_freq_base_update(mod_matrix_sink_t* sink, void* data)
{
	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;
	param_sink->synth_model->lfo_source.lfo.oscillator.frequency = param_sink->synth_model->lfo_def.oscillator.frequency;
}

static void lfo_freq_model_update(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	static const fixed_t freq_base 	= LFO_MIN_FREQUENCY;
	static const fixed_t freq_range	= LFO_MAX_FREQUENCY - LFO_MIN_FREQUENCY;

	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;

	mod_matrix_value_t source_value = source->get_value(source, MOD_MATRIX_SINGLE_SOURCE);

	if (source_value > 0)
	{
		param_sink->synth_model->lfo_source.lfo.oscillator.frequency = freq_base + (fixed_mul_at(freq_range, source_value, MOD_MATRIX_PRECISION));
	}
	else
	{
		param_sink->synth_model->lfo_source.lfo.oscillator.frequency = freq_base;
	}
}

//-------------------------------------------------------------------------------------------------------------------------
// Voice modulation
//
static void voice_amplitude_base_update(mod_matrix_sink_t* sink, void* data)
{
	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;
	voice_t* voice = param_sink->synth_model->voice;

	for (int i = 0; i < param_sink->synth_model->voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			voice->oscillator.level = LEVEL_MAX;
		}
		else
		{
			voice->oscillator.level = 0;
		}
	}
}

static void voice_amplitude_model_update(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;
	voice_t* voice = param_sink->synth_model->voice;

	for (int i = 0; i < param_sink->synth_model->voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			mod_matrix_value_t source_value = source->get_value(source, i);

			if (source_value > 0)
			{
				voice->oscillator.level = (voice->oscillator.level * source_value) / MOD_MATRIX_ONE;
			}
			else
			{
				voice->oscillator.level = 0;
			}
		}
	}
}

static void voice_pitch_base_update(mod_matrix_sink_t* sink, void* data)
{
	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;

	voice_t* voice = param_sink->synth_model->voice;
	for (int i = 0; i < param_sink->synth_model->voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			voice->oscillator.frequency = voice->frequency;
		}
	}
}

static void voice_pitch_model_update(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;

	voice_t* voice = param_sink->synth_model->voice;
	for (int i = 0; i < param_sink->synth_model->voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			mod_matrix_value_t source_value = source->get_value(source, i);
			// TODO: use a proper fixed point power function!
			voice->oscillator.frequency = fixed_mul(voice->oscillator.frequency, powf(2.0f, (float)source_value / (float)MOD_MATRIX_ONE) * FIXED_ONE);
		}
	}
}

static void voice_filter_q_model_update(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	static const fixed_t filter_q_base 	= FILTER_MIN_Q;
	static const fixed_t filter_q_range	= FILTER_MAX_Q - FILTER_MIN_Q;

	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;

	voice_t* voice = param_sink->synth_model->voice;
	for (int i = 0; i < param_sink->synth_model->voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			mod_matrix_value_t source_value = source->get_value(source, i);
			if (source_value > 0)
			{
				voice->filter_def.q = filter_q_base + (fixed_mul_at(filter_q_range, source_value, MOD_MATRIX_PRECISION));
			}
			else
			{
				voice->filter_def.q = filter_q_base;
			}
		}
	}
}

static void voice_filter_freq_model_update(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	static const fixed_t filter_freq_base 	= FILTER_MIN_FREQUENCY;
	static const fixed_t filter_freq_range	= FILTER_MAX_FREQUENCY - FILTER_MIN_FREQUENCY;

	synth_model_param_sink_t* param_sink = (synth_model_param_sink_t*)sink;

	voice_t* voice = param_sink->synth_model->voice;
	for (int i = 0; i < param_sink->synth_model->voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			mod_matrix_value_t source_value = source->get_value(source, i);
			if (source_value > 0)
			{
				voice->filter_def.frequency = filter_freq_base + (fixed_mul_at(filter_freq_range, source_value, MOD_MATRIX_PRECISION));
			}
			else
			{
				voice->filter_def.frequency = filter_freq_base;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------
// Envelope modulation
//

// Certain values in here are also set on the relative controller defaults in synth_controllers.c - this should
// ultimately be reworked so that the default values are only specified in one place (ideally config files) and
// propagate through to the right parts of the synth model during initialisation.
envelope_stage_t envelope_stages[SYNTH_ENVELOPE_COUNT][4] =
{
	{
		{ 0,					MOD_MATRIX_ONE,		100, 			},
		{ MOD_MATRIX_ONE,		MOD_MATRIX_ONE / 2,	250				},
		{ MOD_MATRIX_ONE / 2,	MOD_MATRIX_ONE / 2,	DURATION_HELD 	},
		{ LEVEL_CURRENT,		0,				100				}
	},

	{
		{ 0,					MOD_MATRIX_ONE,		100, 			},
		{ MOD_MATRIX_ONE,		MOD_MATRIX_ONE / 2,	250				},
		{ MOD_MATRIX_ONE / 2,	MOD_MATRIX_ONE / 2,	DURATION_HELD 	},
		{ LEVEL_CURRENT,		0,				100				}
	},

	{
		{ 0,					MOD_MATRIX_ONE,		100, 			},
		{ MOD_MATRIX_ONE,		MOD_MATRIX_ONE / 2,	250				},
		{ MOD_MATRIX_ONE / 2,	MOD_MATRIX_ONE / 2,	DURATION_HELD 	},
		{ LEVEL_CURRENT,		0,				100				}
	},
};

static void envelope_generate_value(mod_matrix_source_t* source, void* data)
{
	envelope_source_t* envelope_source = (envelope_source_t*)source;
	synth_update_state_t* state = (synth_update_state_t*)data;

	for (int i = 0; i < SYNTH_GLOBAL_ENVELOPE_INSTANCE_COUNT + state->synth_model->voice_count; i++)
	{
		envelope_step(envelope_source->envelope_instance + i, state->timestep_ms);
	}
}

static mod_matrix_value_t envelope_get_value(mod_matrix_source_t* source, int subsource_id)
{
	envelope_source_t* envelope_source = (envelope_source_t*)source;
	return (mod_matrix_value_t) envelope_source->envelope_instance[subsource_id + 1].last_level;
}

void synth_model_start_global_envelopes(synth_model_t* synth_model)
{
	synth_model->global_envelopes_released = FALSE;
	envelope_start(synth_model->envelope_source[0].envelope_instance + SYNTH_GLOBAL_ENVELOPE_INSTANCE);
	envelope_start(synth_model->envelope_source[1].envelope_instance + SYNTH_GLOBAL_ENVELOPE_INSTANCE);
	envelope_start(synth_model->envelope_source[2].envelope_instance + SYNTH_GLOBAL_ENVELOPE_INSTANCE);
}

void synth_model_start_voice_envelopes(synth_model_t* synth_model, int voice_index)
{
	envelope_start(synth_model->envelope_source[0].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice_index);
	envelope_start(synth_model->envelope_source[1].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice_index);
	envelope_start(synth_model->envelope_source[2].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice_index);
}

void synth_model_release_global_envelopes(synth_model_t* synth_model)
{
	synth_model->global_envelopes_released = TRUE;
	envelope_go_to_stage(synth_model->envelope_source[0].envelope_instance + SYNTH_GLOBAL_ENVELOPE_INSTANCE, ENVELOPE_STAGE_RELEASE);
	envelope_go_to_stage(synth_model->envelope_source[1].envelope_instance + SYNTH_GLOBAL_ENVELOPE_INSTANCE, ENVELOPE_STAGE_RELEASE);
	envelope_go_to_stage(synth_model->envelope_source[2].envelope_instance + SYNTH_GLOBAL_ENVELOPE_INSTANCE, ENVELOPE_STAGE_RELEASE);
}

void synth_model_release_voice_envelopes(synth_model_t* synth_model, int voice_index)
{
	envelope_go_to_stage(synth_model->envelope_source[0].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice_index, ENVELOPE_STAGE_RELEASE);
	envelope_go_to_stage(synth_model->envelope_source[1].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice_index, ENVELOPE_STAGE_RELEASE);
	envelope_go_to_stage(synth_model->envelope_source[2].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice_index, ENVELOPE_STAGE_RELEASE);
}

void synth_model_init_envelopes(synth_model_t* synth_model, int voice_count)
{
	envelopes_initialise();

	synth_model->envelope[0].peak = MOD_MATRIX_ONE;
	synth_model->envelope[0].stage_count = 4;
	synth_model->envelope[0].stages = envelope_stages[0];

	synth_model->envelope[1].peak = MOD_MATRIX_ONE;
	synth_model->envelope[1].stage_count = 4;
	synth_model->envelope[1].stages = envelope_stages[1];

	synth_model->envelope[2].peak = MOD_MATRIX_ONE;
	synth_model->envelope[2].stage_count = 4;
	synth_model->envelope[2].stages = envelope_stages[2];

	synth_model->envelope_instances = calloc((voice_count + SYNTH_GLOBAL_ENVELOPE_INSTANCE_COUNT) * SYNTH_ENVELOPE_COUNT, sizeof(envelope_instance_t));

	for (int i = 0; i < SYNTH_ENVELOPE_COUNT; i++)
	{
		synth_model->envelope_source[i].envelope_instance = synth_model->envelope_instances + ((voice_count + SYNTH_GLOBAL_ENVELOPE_INSTANCE_COUNT) * i);
	}

	for (int i = 0; i < voice_count + SYNTH_GLOBAL_ENVELOPE_INSTANCE_COUNT; i++)
	{
		envelope_init(synth_model->envelope_source[0].envelope_instance + i, &synth_model->envelope[0]);
		envelope_init(synth_model->envelope_source[1].envelope_instance + i, &synth_model->envelope[1]);
		envelope_init(synth_model->envelope_source[2].envelope_instance + i, &synth_model->envelope[2]);
	}

	mod_matrix_init_source(SYNTH_MOD_SOURCE_ENVELOPE_1, envelope_generate_value, envelope_get_value, &synth_model->envelope_source[0].source);
	mod_matrix_init_source(SYNTH_MOD_SOURCE_ENVELOPE_2, envelope_generate_value, envelope_get_value, &synth_model->envelope_source[1].source);
	mod_matrix_init_source(SYNTH_MOD_SOURCE_ENVELOPE_3, envelope_generate_value, envelope_get_value, &synth_model->envelope_source[2].source);

	mod_matrix_add_source(&synth_model->envelope_source[0].source);
	mod_matrix_add_source(&synth_model->envelope_source[1].source);
	mod_matrix_add_source(&synth_model->envelope_source[2].source);
}

void synth_model_deinit_envelopes(synth_model_t* synth_model)
{
	free(synth_model->envelope_instances);
	synth_model->envelope_instances = NULL;
}

//=========================================================================================================================
// External callbacks
//
static void voice_event_callback(voice_event_t callback_event, voice_t* voice, void* callback_data)
{
	synth_model_t* synth_model = (synth_model_t*)callback_data;

	switch(callback_event)
	{
		case VOICE_EVENT_VOICE_STARTING:
		{
			if (synth_model->active_voices == 0)
			{
				lfo_reset(&synth_model->lfo_source.lfo);
				synth_model_start_global_envelopes(synth_model);
			}
			else if (synth_model->global_envelopes_released)
			{
				synth_model_start_global_envelopes(synth_model);
			}

			synth_model->active_voices++;
			break;
		}

		case VOICE_EVENT_NOTE_STARTING:
		{
			synth_model_start_voice_envelopes(synth_model, voice->index);
			break;
		}

		case VOICE_EVENT_NOTE_ENDING:
		{
			synth_model->ending_voices++;

			if (synth_model->voice_amplitude_envelope_count > 0)
			{
				synth_model_release_voice_envelopes(synth_model, voice->index);

				if (synth_model->ending_voices == synth_model->active_voices)
				{
					synth_model_release_global_envelopes(synth_model);
				}
			}
			else
			{
				voice_kill(voice);
			}
			break;
		}

		case VOICE_EVENT_VOICE_ENDED:
		default:
		{
			synth_model->ending_voices--;
			synth_model->active_voices--;
			break;
		}
	}
}

static void mod_matrix_callback(mod_matrix_event_t callback_event, mod_matrix_source_t* source, mod_matrix_sink_t* sink, void* callback_data)
{
	synth_model_t* synth_model = (synth_model_t*)callback_data;

	if (sink == &synth_model->voice_amplitude_sink.sink)
	{
		for (int i = 0; i < SYNTH_ENVELOPE_COUNT; i++)
		{
			if (source == &synth_model->envelope_source[i].source)
			{
				if (callback_event == MOD_MATRIX_EVENT_CONNECTION)
				{
					synth_model->voice_amplitude_envelope_count++;
				}
				else if (callback_event == MOD_MATRIX_EVENT_DISCONNECTION)
				{
					synth_model->voice_amplitude_envelope_count--;
				}
			}
		}
	}
}

//=========================================================================================================================
// Synth model entrypoints
//
void synth_model_initialise(synth_model_t* synth_model, int voice_count)
{
	synth_model->voice_count 					= voice_count;
	synth_model->active_voices					= 0;
	synth_model->ending_voices					= 0;
	synth_model->global_envelopes_released		= FALSE;
	synth_model->voice_amplitude_envelope_count	= 0;

	synth_model->voice = (voice_t*)calloc(synth_model->voice_count, sizeof(voice_t));
	voices_initialise(synth_model->voice, synth_model->voice_count);

	synth_model_init_param_sink(SYNTH_MOD_SINK_NOTE_AMPLITUDE, voice_amplitude_base_update, voice_amplitude_model_update, synth_model, &synth_model->voice_amplitude_sink);
	synth_model_init_param_sink(SYNTH_MOD_SINK_NOTE_PITCH, voice_pitch_base_update, voice_pitch_model_update, synth_model, &synth_model->voice_pitch_sink);
	synth_model_init_param_sink(SYNTH_MOD_SINK_FILTER_Q, NULL, voice_filter_q_model_update, synth_model, &synth_model->voice_filter_q_sink);
	synth_model_init_param_sink(SYNTH_MOD_SINK_FILTER_FREQ, NULL, voice_filter_freq_model_update, synth_model, &synth_model->voice_filter_freq_sink);
	synth_model_init_param_sink(SYNTH_MOD_SINK_LFO_AMPLITUDE, lfo_amplitude_base_update, lfo_amplitude_model_update, synth_model, &synth_model->lfo_amplitude_sink);
	synth_model_init_param_sink(SYNTH_MOD_SINK_LFO_FREQ, lfo_freq_base_update, lfo_freq_model_update, synth_model, &synth_model->lfo_freq_sink);

	synth_model_init_envelopes(synth_model, voice_count);

	lfo_init(&synth_model->lfo_def);
	lfo_init(&synth_model->lfo_source.lfo);
	mod_matrix_init_source(SYNTH_MOD_SOURCE_LFO, lfo_generate_value, lfo_get_value, &synth_model->lfo_source.source);
	mod_matrix_add_source(&synth_model->lfo_source.source);

	voices_add_callback(voice_event_callback, synth_model);
	mod_matrix_add_callback(mod_matrix_callback, synth_model);

	synth_model->global_filter_def.type = FILTER_PASS;
	synth_model->global_filter_def.frequency = 9000 * FILTER_FIXED_ONE;
	synth_model->global_filter_def.q = FIXED_HALF;
}

void synth_model_deinitialise(synth_model_t* synth_model)
{
	voices_remove_callback(voice_event_callback);
	mod_matrix_remove_callback(mod_matrix_callback);
	synth_model_deinit_envelopes(synth_model);
	free(synth_model->voice);
	synth_model->voice = NULL;
}

void synth_model_update(synth_model_t* synth_model, synth_update_state_t* update_state)
{
	update_state->synth_model = synth_model;

	// Update components used in modulation matrix that rely on state not
	// available in the modulation matrix (at least for now).
	for (int i = 0; i < synth_model->voice_count; i++)
	{
		voice_preupdate(synth_model->voice + i, update_state->timestep_ms, &synth_model->global_filter_def);
	}

	mod_matrix_update(update_state);
}

void synth_model_play_note(synth_model_t* synth_model, int channel, unsigned char midi_note)
{
	voice_t *candidate_voice = voice_find_next_likely_free(synth_model->voice, synth_model->voice_count, channel);

	if (candidate_voice != NULL)
	{
		int master_waveform = setting_get_value_enum_as_int(synth_model->setting_master_waveform);
		voice_play_note(candidate_voice, midi_note, master_waveform);
	}
}

void synth_model_stop_note(synth_model_t* synth_model, int channel, unsigned char midi_note)
{
	voice_t *playing_voice = voice_find_playing_note(synth_model->voice, synth_model->voice_count, channel, midi_note);

	if (playing_voice != NULL)
	{
		voice_stop_note(playing_voice);
	}
}
