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
#include "modulation_matrix.h"
#include "voice.h"
#include "lfo.h"

const char*	SYNTH_MOD_SOURCE_LFO			= "lfo";
const char*	SYNTH_MOD_SINK_NOTE_AMPLITUDE	= "note-amplitude";
const char*	SYNTH_MOD_SINK_NOTE_PITCH		= "note-pitch";

typedef struct voice_array_t
{
	int			voice_count;
	voice_t*	voices;
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

void voice_amplitude_base_update(mod_matrix_sink_t* sink)
{
	voice_sink_t* voice_sink = (voice_sink_t*)sink;

	voice_t* voice = voice_sink->voice_array.voices;
	for (int i = 0; i < voice_sink->voice_array.voice_count; i++, voice++)
	{
		if (voice->current_state != NOTE_NOT_PLAYING)
		{
			voice->oscillator.level = voice->level_envelope_instance.last_level;
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
			if (source->value > 0)
			{
				voice->oscillator.level = (voice->oscillator.level * source->value) / MOD_MATRIX_ONE;
			}
			else
			{
				voice->oscillator.level = 0;
			}
		}
	}
}

void voice_pitch_base_update(mod_matrix_sink_t* sink)
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
			// TODO: use a proper fixed point power function!
			voice->oscillator.frequency = fixed_mul(voice->oscillator.frequency, powf(2.0f, (float)source->value / (float)MOD_MATRIX_ONE) * FIXED_ONE);
		}
	}
}

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
	LOG_INFO("Voice event %d for %08x (%08x)", callback_event, voice, callback_data);
}

void synth_model_initialise(synth_model_t* synth_model, int voice_count)
{
	synth_model->voice_count 	= voice_count;
	synth_model->active_voices	= 0;
	synth_model->voice 			= (voice_t*)calloc(synth_model->voice_count, sizeof(voice_t));
	voice_init(synth_model->voice, synth_model->voice_count, &synth_model->envelope[0], &synth_model->envelope[1], &synth_model->envelope[2]);
	voice_add_callback(voice_event_callback, NULL);

	init_voice_sink(SYNTH_MOD_SINK_NOTE_AMPLITUDE, voice_amplitude_base_update, voice_amplitude_model_update, voice_count, synth_model->voice, &voice_amplitude_sink);
	init_voice_sink(SYNTH_MOD_SINK_NOTE_PITCH, voice_pitch_base_update, voice_pitch_model_update, voice_count, synth_model->voice, &voice_pitch_sink);

	lfo_init(&synth_model->lfo, SYNTH_MOD_SOURCE_LFO);

	synth_model->global_filter_def.type = FILTER_PASS;
	synth_model->global_filter_def.frequency = 9000 * FILTER_FIXED_ONE;
	synth_model->global_filter_def.q = FIXED_HALF;

	synth_model->envelope[0].peak = LEVEL_MAX;
	synth_model->envelope[0].stage_count = 4;
	synth_model->envelope[0].stages = envelope_stages;

	synth_model->envelope[1].peak = FILTER_FIXED_ONE * 18000;
	synth_model->envelope[1].stage_count = 4;
	synth_model->envelope[1].stages = freq_envelope_stages;

	synth_model->envelope[2].peak = FIXED_ONE;
	synth_model->envelope[2].stage_count = 4;
	synth_model->envelope[2].stages = q_envelope_stages;
}

void synth_model_deinitialise(synth_model_t* synth_model)
{
	voice_remove_callback(voice_event_callback);
	free(synth_model->voice);
	synth_model->voice = NULL;
}
