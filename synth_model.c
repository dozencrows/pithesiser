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
#define VOICE_MOD_FLAG_ENDING	0x01

typedef struct voice_array_t
{
	int				voice_count;
	voice_t*		voices;
} voice_array_t;

typedef struct voice_sink_t
{
	mod_matrix_sink_t	sink;
	voice_array_t		voice_array;
} voice_sink_t;

void init_voice_sink(const char* name, base_update_t base_update, model_update_t model_update, int voice_count, voice_t* voices, voice_sink_t* sink)
{
	mod_matrix_init_sink(name, base_update, model_update, &sink->sink);
	mod_matrix_add_sink(&sink->sink);
	sink->voice_array.voice_count = voice_count;
	sink->voice_array.voices = voices;
}

voice_sink_t voice_amplitude_sink;
voice_sink_t voice_pitch_sink;

void voice_amplitude_base_update(mod_matrix_sink_t* sink, void* data)
{
	voice_sink_t* voice_sink = (voice_sink_t*)sink;
	voice_t* voice = voice_sink->voice_array.voices;

	for (int i = 0; i < voice_sink->voice_array.voice_count; i++, voice++)
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
	voice_t* voice = voice_sink->voice_array.voices;

	for (int i = 0; i < voice_sink->voice_array.voice_count; i++, voice++)
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

	voice_t* voice = voice_sink->voice_array.voices;
	for (int i = 0; i < voice_sink->voice_array.voice_count; i++, voice++)
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

	voice_t* voice = voice_sink->voice_array.voices;
	for (int i = 0; i < voice_sink->voice_array.voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			mod_matrix_value_t source_value = source->get_value(source, i);
			// TODO: use a proper fixed point power function!
			voice->oscillator.frequency = fixed_mul(voice->oscillator.frequency, powf(2.0f, (float)source_value / (float)MOD_MATRIX_ONE) * FIXED_ONE);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------
// Synth envelopes
//

// Certain values in here are also set on the relative controller defaults in synth_controllers.c - this should
// ultimately be reworked so that the default values are only specified in one place (ideally config files) and
// propagate through to the right parts of the synth model during initialisation.
envelope_stage_t envelope_stages[4] =
{
	{ 0,				LEVEL_MAX,		100, 			},
	{ LEVEL_MAX,		LEVEL_MAX / 2,	250				},
	{ LEVEL_MAX / 2,	LEVEL_MAX / 2,	DURATION_HELD 	},
	{ LEVEL_CURRENT,	0,				100				}
};

envelope_stage_t freq_envelope_stages[4] =
{
	{ FILTER_FIXED_ONE * 20,	FILTER_FIXED_ONE * 12000,	1000, 			},
	{ FILTER_FIXED_ONE * 12000,	FILTER_FIXED_ONE * 12000,	1				},
	{ FILTER_FIXED_ONE * 12000,	FILTER_FIXED_ONE * 12000,	DURATION_HELD 	},
	{ LEVEL_CURRENT,			FILTER_FIXED_ONE * 20,		200				}
};

envelope_stage_t q_envelope_stages[4] =
{
	{ FIXED_ONE / 100,	FIXED_ONE * .75,	1000, 			},
	{ FIXED_ONE * .75,	FIXED_ONE * .75,	1				},
	{ FIXED_ONE * .75,	FIXED_ONE * .75,	DURATION_HELD 	},
	{ LEVEL_CURRENT,	FIXED_ONE / 100,	200				}
};

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
			}

			synth_model->active_voices++;
			break;
		}

		case VOICE_EVENT_NOTE_STARTING:
		{
			envelope_start(synth_model->envelope_instance[0] + voice->index);
			envelope_start(synth_model->envelope_instance[1] + voice->index);
			envelope_start(synth_model->envelope_instance[2] + voice->index);
			break;
		}

		case VOICE_EVENT_NOTE_ENDING:
		{
			// TODO:
			// Check for envelope assignment to voice amplitude sink
			// If none present, stop voice and envelopes dead
			// Else switch all envelopes to release
			//envelope_go_to_stage(synth_model->envelope_instance[0] + voice->index, ENVELOPE_STAGE_RELEASE);
			//envelope_go_to_stage(synth_model->envelope_instance[1] + voice->index, ENVELOPE_STAGE_RELEASE);
			//envelope_go_to_stage(synth_model->envelope_instance[2] + voice->index, ENVELOPE_STAGE_RELEASE);
			voice_kill(voice);
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

void envelope_generate_value(mod_matrix_source_t* source)
{
}

mod_matrix_value_t envelope_get_value(mod_matrix_source_t* source, int subsource_id)
{
	return (mod_matrix_value_t) MOD_MATRIX_ONE;
}

void synth_model_init_envelopes(synth_model_t* synth_model, int voice_count)
{
	envelopes_initialise();
	voices_add_callback(voice_event_callback, synth_model);
	envelopes_add_callback(envelope_event_callback, synth_model);

	synth_model->envelope[0].peak = LEVEL_MAX;
	synth_model->envelope[0].stage_count = 4;
	synth_model->envelope[0].stages = envelope_stages;

	synth_model->envelope[1].peak = FILTER_FIXED_ONE * 18000;
	synth_model->envelope[1].stage_count = 4;
	synth_model->envelope[1].stages = freq_envelope_stages;

	synth_model->envelope[2].peak = FIXED_ONE;
	synth_model->envelope[2].stage_count = 4;
	synth_model->envelope[2].stages = q_envelope_stages;

	synth_model->envelope_instance[0] = calloc(voice_count * SYNTH_ENVELOPE_COUNT, sizeof(envelope_instance_t));

	for (int i = 1; i < SYNTH_ENVELOPE_COUNT; i++)
	{
		synth_model->envelope_instance[i] = synth_model->envelope_instance[0] + (voice_count * i);
	}

	for (int i = 0; i < voice_count; i++)
	{
		envelope_init(synth_model->envelope_instance[0] + i, &synth_model->envelope[0]);
		envelope_init(synth_model->envelope_instance[1] + i, &synth_model->envelope[1]);
		envelope_init(synth_model->envelope_instance[2] + i, &synth_model->envelope[2]);
	}

	//mod_matrix_init_source(name, lfo_generate_value, &lfo->mod_matrix_source);
	//mod_matrix_add_source(&lfo->mod_matrix_source);
}

void synth_model_deinit_envelopes(synth_model_t* synth_model)
{
	voices_remove_callback(voice_event_callback);
	envelopes_remove_callback(envelope_event_callback);
	free(synth_model->envelope_instance[0]);
}

//-------------------------------------------------------------------------------------------------------------------------
// Synth model
//

void synth_model_initialise(synth_model_t* synth_model, int voice_count)
{
	synth_model->voice_count 	= voice_count;
	synth_model->active_voices	= 0;
	synth_model->voice 			= (voice_t*)calloc(synth_model->voice_count, sizeof(voice_t));
	voices_initialise(synth_model->voice, synth_model->voice_count);

	init_voice_sink(SYNTH_MOD_SINK_NOTE_AMPLITUDE, voice_amplitude_base_update, voice_amplitude_model_update, voice_count, synth_model->voice, &voice_amplitude_sink);
	init_voice_sink(SYNTH_MOD_SINK_NOTE_PITCH, voice_pitch_base_update, voice_pitch_model_update, voice_count, synth_model->voice, &voice_pitch_sink);

	synth_model_init_envelopes(synth_model, voice_count);

	lfo_init(&synth_model->lfo_source.lfo);
	mod_matrix_init_source(SYNTH_MOD_SOURCE_LFO, lfo_generate_value, lfo_get_value, &synth_model->lfo_source.source);
	mod_matrix_add_source(&synth_model->lfo_source.source);

	synth_model->global_filter_def.type = FILTER_PASS;
	synth_model->global_filter_def.frequency = 9000 * FILTER_FIXED_ONE;
	synth_model->global_filter_def.q = FIXED_HALF;
}

void synth_model_deinitialise(synth_model_t* synth_model)
{
	synth_model_deinit_envelopes(synth_model);
	free(synth_model->voice);
	synth_model->voice = NULL;
}

void synth_model_update(synth_model_t* synth_model, synth_update_state_t* update_state)
{
	// Update components used in modulation matrix that rely on state not
	// available in the modulation matrix (at least for now).
	for (int i = 0; i < synth_model->voice_count; i++)
	{
		voice_preupdate(synth_model->voice + i, update_state->timestep_ms, &synth_model->global_filter_def);
	}

	mod_matrix_update(update_state);
}
