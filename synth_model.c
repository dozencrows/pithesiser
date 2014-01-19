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

const char*	SYNTH_MOD_SOURCE_LFO			= "lfo";
const char*	SYNTH_MOD_SOURCE_ENVELOPE_1		= "envelope-1";
const char*	SYNTH_MOD_SOURCE_ENVELOPE_2		= "envelope-2";
const char*	SYNTH_MOD_SOURCE_ENVELOPE_3		= "envelope-3";
const char*	SYNTH_MOD_SINK_NOTE_AMPLITUDE	= "note-amplitude";
const char*	SYNTH_MOD_SINK_NOTE_PITCH		= "note-pitch";
const char*	SYNTH_MOD_SINK_FILTER_Q			= "filter-q";

//-------------------------------------------------------------------------------------------------------------------------
// LFO modelling
//
void lfo_generate_value(mod_matrix_source_t* source, void* data)
{
	lfo_source_t* lfo_source = (lfo_source_t*)source;
	synth_update_state_t* state = (synth_update_state_t*)data;

	lfo_update(&lfo_source->lfo, state->sample_count);
}

mod_matrix_value_t lfo_get_value(mod_matrix_source_t* source, int subsource_id)
{
	lfo_source_t* lfo_source = (lfo_source_t*)source;
	return (mod_matrix_value_t) lfo_source->lfo.value;
}

//-------------------------------------------------------------------------------------------------------------------------
// Synth voices
//
void init_voice_sink(const char* name, base_update_t base_update, model_update_t model_update, int voice_count, voice_t* voices, voice_sink_t* sink)
{
	mod_matrix_init_sink(name, base_update, model_update, &sink->sink);
	mod_matrix_add_sink(&sink->sink);
	sink->voice_count = voice_count;
	sink->voices = voices;
}

void voice_amplitude_base_update(mod_matrix_sink_t* sink, void* data)
{
	voice_sink_t* voice_sink = (voice_sink_t*)sink;
	voice_t* voice = voice_sink->voices;

	for (int i = 0; i < voice_sink->voice_count; i++, voice++)
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

void voice_amplitude_model_update(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	voice_sink_t* voice_sink = (voice_sink_t*)sink;
	voice_t* voice = voice_sink->voices;

	for (int i = 0; i < voice_sink->voice_count; i++, voice++)
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

void voice_pitch_base_update(mod_matrix_sink_t* sink, void* data)
{
	voice_sink_t* voice_sink = (voice_sink_t*)sink;

	voice_t* voice = voice_sink->voices;
	for (int i = 0; i < voice_sink->voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			voice->oscillator.frequency = voice->frequency;
		}
	}
}

void voice_pitch_model_update(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	voice_sink_t* voice_sink = (voice_sink_t*)sink;

	voice_t* voice = voice_sink->voices;
	for (int i = 0; i < voice_sink->voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			mod_matrix_value_t source_value = source->get_value(source, i);
			// TODO: use a proper fixed point power function!
			voice->oscillator.frequency = fixed_mul(voice->oscillator.frequency, powf(2.0f, (float)source_value / (float)MOD_MATRIX_ONE) * FIXED_ONE);
		}
	}
}

void voice_filter_q_model_update(mod_matrix_source_t* source, mod_matrix_sink_t* sink)
{
	static const fixed_t filter_q_base 	= FILTER_MIN_Q;
	static const fixed_t filter_q_range	= FILTER_MAX_Q - FILTER_MIN_Q;

	voice_sink_t* voice_sink = (voice_sink_t*)sink;

	voice_t* voice = voice_sink->voices;
	for (int i = 0; i < voice_sink->voice_count; i++, voice++)
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

void voice_event_callback(voice_event_t callback_event, voice_t* voice, void* callback_data)
{
	synth_model_t* synth_model = (synth_model_t*)callback_data;

	//LOG_INFO("Voice event %d for %08x (%08x)", callback_event, voice, callback_data);
	switch(callback_event)
	{
		case VOICE_EVENT_VOICE_STARTING:
		{
			if (synth_model->active_voices == 0)
			{
				lfo_reset(&synth_model->lfo_source.lfo);
				//envelope_start(synth_model->envelope_source[0].envelope_instance + SYNTH_GLOBAL_ENVELOPE_INSTANCE);
				//envelope_start(synth_model->envelope_source[1].envelope_instance + SYNTH_GLOBAL_ENVELOPE_INSTANCE);
				//envelope_start(synth_model->envelope_source[2].envelope_instance + SYNTH_GLOBAL_ENVELOPE_INSTANCE);
			}

			synth_model->active_voices++;
			break;
		}

		case VOICE_EVENT_NOTE_STARTING:
		{
			envelope_start(synth_model->envelope_source[0].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice->index);
			envelope_start(synth_model->envelope_source[1].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice->index);
			envelope_start(synth_model->envelope_source[2].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice->index);
			break;
		}

		case VOICE_EVENT_NOTE_ENDING:
		{
			if (synth_model->voice_amplitude_envelope_count > 0)
			{
				envelope_go_to_stage(synth_model->envelope_source[0].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice->index, ENVELOPE_STAGE_RELEASE);
				envelope_go_to_stage(synth_model->envelope_source[1].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice->index, ENVELOPE_STAGE_RELEASE);
				envelope_go_to_stage(synth_model->envelope_source[2].envelope_instance + SYNTH_VOICE_ENVELOPE_INSTANCE_BASE + voice->index, ENVELOPE_STAGE_RELEASE);
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
			synth_model->active_voices--;
			break;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------
// Synth envelopes
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

//envelope_stage_t freq_envelope_stages[4] = // peak FILTER_FIXED_ONE * 18000;
//{
//	{ FILTER_FIXED_ONE * 20,	FILTER_FIXED_ONE * 12000,	1000, 			},
//	{ FILTER_FIXED_ONE * 12000,	FILTER_FIXED_ONE * 12000,	1				},
//	{ FILTER_FIXED_ONE * 12000,	FILTER_FIXED_ONE * 12000,	DURATION_HELD 	},
//	{ LEVEL_CURRENT,			FILTER_FIXED_ONE * 20,		200				}
//};
//
//envelope_stage_t q_envelope_stages[4] =	// peak FIXED_ONE
//{
//	{ FIXED_ONE / 100,	FIXED_ONE * .75,	1000, 			},
//	{ FIXED_ONE * .75,	FIXED_ONE * .75,	1				},
//	{ FIXED_ONE * .75,	FIXED_ONE * .75,	DURATION_HELD 	},
//	{ LEVEL_CURRENT,	FIXED_ONE / 100,	200				}
//};

void envelope_event_callback(envelope_event_t callback_event, envelope_instance_t* envelope, void* callback_data)
{
	//LOG_INFO("Envelope event %d for %08x (%08x)", callback_event, envelope, callback_data);
	switch(callback_event)
	{
		case ENVELOPE_EVENT_STARTING:
		case ENVELOPE_EVENT_STAGE_CHANGE:
		default:
		{
			break;
		}

		case ENVELOPE_EVENT_COMPLETED:
		{
			break;
		}
	}
}

void envelope_generate_value(mod_matrix_source_t* source, void* data)
{
	envelope_source_t* envelope_source = (envelope_source_t*)source;
	synth_update_state_t* state = (synth_update_state_t*)data;

	for (int i = 0; i < SYNTH_GLOBAL_ENVELOPE_INSTANCE_COUNT + state->synth_model->voice_count; i++)
	{
		envelope_step(envelope_source->envelope_instance + i, state->timestep_ms);
	}
}

mod_matrix_value_t envelope_get_value(mod_matrix_source_t* source, int subsource_id)
{
	envelope_source_t* envelope_source = (envelope_source_t*)source;
	return (mod_matrix_value_t) envelope_source->envelope_instance[subsource_id + 1].last_level;
}

void synth_model_init_envelopes(synth_model_t* synth_model, int voice_count)
{
	envelopes_initialise();
	voices_add_callback(voice_event_callback, synth_model);
	envelopes_add_callback(envelope_event_callback, synth_model);

	synth_model->envelope[0].peak = MOD_MATRIX_ONE;
	synth_model->envelope[0].stage_count = 4;
	synth_model->envelope[0].stages = envelope_stages[0];

	synth_model->envelope[1].peak = MOD_MATRIX_ONE;
	synth_model->envelope[1].stage_count = 4;
	synth_model->envelope[1].stages = envelope_stages[1];

	synth_model->envelope[2].peak = MOD_MATRIX_ONE;
	synth_model->envelope[2].stage_count = 4;
	synth_model->envelope[2].stages = envelope_stages[2];

	// One global instance plus one instance per voice for each envelope
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
	voices_remove_callback(voice_event_callback);
	envelopes_remove_callback(envelope_event_callback);
	free(synth_model->envelope_instances);
}

void synth_model_mod_matrix_callback(mod_matrix_event_t callback_event, mod_matrix_source_t* source, mod_matrix_sink_t* sink, void* callback_data)
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

//-------------------------------------------------------------------------------------------------------------------------
// Synth model
//
void synth_model_initialise(synth_model_t* synth_model, int voice_count)
{
	synth_model->voice_count 					= voice_count;
	synth_model->active_voices					= 0;
	synth_model->voice_amplitude_envelope_count	= 0;

	synth_model->voice = (voice_t*)calloc(synth_model->voice_count, sizeof(voice_t));
	voices_initialise(synth_model->voice, synth_model->voice_count);

	init_voice_sink(SYNTH_MOD_SINK_NOTE_AMPLITUDE, voice_amplitude_base_update, voice_amplitude_model_update, voice_count, synth_model->voice, &synth_model->voice_amplitude_sink);
	init_voice_sink(SYNTH_MOD_SINK_NOTE_PITCH, voice_pitch_base_update, voice_pitch_model_update, voice_count, synth_model->voice, &synth_model->voice_pitch_sink);
	init_voice_sink(SYNTH_MOD_SINK_FILTER_Q, NULL, voice_filter_q_model_update, voice_count, synth_model->voice, &synth_model->voice_filter_q_sink);

	synth_model_init_envelopes(synth_model, voice_count);

	lfo_init(&synth_model->lfo_source.lfo);
	mod_matrix_init_source(SYNTH_MOD_SOURCE_LFO, lfo_generate_value, lfo_get_value, &synth_model->lfo_source.source);
	mod_matrix_add_source(&synth_model->lfo_source.source);

	mod_matrix_add_callback(synth_model_mod_matrix_callback, synth_model);

	synth_model->global_filter_def.type = FILTER_PASS;
	synth_model->global_filter_def.frequency = 9000 * FILTER_FIXED_ONE;
	synth_model->global_filter_def.q = FIXED_HALF;
}

void synth_model_deinitialise(synth_model_t* synth_model)
{
	mod_matrix_remove_callback(synth_model_mod_matrix_callback);
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
