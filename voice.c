/*
 * voice.c
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#include "voice.h"
#include <stddef.h>
#include <math.h>
#include "midi.h"
#include "lfo.h"
#include "modulation_matrix.h"
#include "fixed_point_math.h"

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
			voice->oscillator.level = (voice->oscillator.level * source->value) / MOD_MATRIX_ONE;
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

static void init_voice(voice_t *voice, envelope_t *level_envelope, envelope_t *filter_freq_envelope, envelope_t *filter_q_envelope)
{
	voice->midi_channel = 0;
	voice->last_state = NOTE_NOT_PLAYING;
	voice->current_state = NOTE_NOT_PLAYING;
	voice->play_counter = 0;

	osc_init(&voice->oscillator);
	envelope_init(&voice->level_envelope_instance, level_envelope);
	envelope_init(&voice->filter_freq_envelope_instance, filter_freq_envelope);
	envelope_init(&voice->filter_q_envelope_instance, filter_q_envelope);
	filter_init(&voice->filter);
	voice->filter_def = voice->filter.definition;
}

void voice_init(voice_t *voices, int voice_count, envelope_t *level_envelope, envelope_t *filter_freq_envelope, envelope_t *filter_q_envelope)
{
	init_voice_sink("note-amplitude", voice_amplitude_base_update, voice_amplitude_model_update, voice_count, voices, &voice_amplitude_sink);
	init_voice_sink("note-pitch", voice_pitch_base_update, voice_pitch_model_update, voice_count, voices, &voice_pitch_sink);

	for (int i = 0; i < voice_count; i++)
	{
		init_voice(voices + i, level_envelope, filter_freq_envelope, filter_q_envelope);
	}
}

void voice_preupdate(voice_t *voice, int32_t timestep_ms, filter_definition_t *filter_def)
{
	if (voice->current_state != voice->last_state)
	{
		if (voice->current_state == NOTE_NOT_PLAYING)
		{
			voice->oscillator.level = 0;
		}
		else if (voice->current_state >= 0)
		{
			//voice->oscillator.frequency = voice->frequency;
			voice->oscillator.phase_accumulator = 0;
			envelope_start(&voice->level_envelope_instance);
			envelope_start(&voice->filter_freq_envelope_instance);
			envelope_start(&voice->filter_q_envelope_instance);
			filter_silence(&voice->filter);
			voice->filter_def = *filter_def;
		}

		if (voice->last_state == NOTE_NOT_PLAYING)
		{
			voice->oscillator.last_level = -1;
		}

		voice->last_state = voice->current_state;
	}

	if (voice->current_state != NOTE_NOT_PLAYING)
	{
		envelope_step(&voice->level_envelope_instance, timestep_ms);
	}
}

int voice_update(voice_t *voice, int32_t master_level, sample_t *voice_buffer, int buffer_samples, int32_t timestep_ms, filter_definition_t *filter_def)
{
	int voice_state = VOICE_IDLE;

	if (voice->current_state != NOTE_NOT_PLAYING)
	{
		voice->oscillator.level = (voice->oscillator.level * master_level) / LEVEL_MAX;

		// If this is a new note from scratch, avoid interpolating the initial level across the chunk.
		if (voice->oscillator.last_level < 0)
		{
			voice->oscillator.last_level = voice->oscillator.level;
		}

		voice->filter_def.frequency = envelope_step(&voice->filter_freq_envelope_instance, timestep_ms);
		voice->filter_def.q = envelope_step(&voice->filter_q_envelope_instance, timestep_ms);
		if (!filter_definitions_same(&voice->filter_def, &voice->filter.definition))
		{
			voice->filter.definition = voice->filter_def;
			filter_update(&voice->filter);
		}

		if (voice->oscillator.level > 0 || voice->oscillator.last_level != 0)
		{
			osc_output(&voice->oscillator, voice_buffer, buffer_samples);
			filter_apply(&voice->filter, voice_buffer, buffer_samples);
			voice->oscillator.last_level = voice->oscillator.level;
			voice_state = VOICE_ACTIVE;
		}
		else if (voice->current_state == NOTE_ENDING || envelope_completed(&voice->level_envelope_instance))
		{
			voice->current_state = NOTE_NOT_PLAYING;
			voice_state = VOICE_GONE_IDLE;
		}
	}

	return voice_state;
}

void voice_play_note(voice_t *voice, int midi_note, waveform_type_t waveform)
{
	voice->note = midi_note;
	voice->current_state = midi_note;
	voice->oscillator.waveform = waveform;
	voice->frequency = midi_get_note_frequency(midi_note);
}

void voice_stop_note(voice_t *voice)
{
	if (PLAYING_NOTE(voice->current_state))
	{
		voice->current_state = NOTE_ENDING;
		envelope_go_to_stage(&voice->level_envelope_instance, ENVELOPE_STAGE_RELEASE);
		envelope_go_to_stage(&voice->filter_freq_envelope_instance, ENVELOPE_STAGE_RELEASE);
		envelope_go_to_stage(&voice->filter_q_envelope_instance, ENVELOPE_STAGE_RELEASE);
	}
}

voice_t *voice_find_next_likely_free(voice_t *voices, int voice_count, int midi_channel, int *voice_state)
{
	int candidate_voice = -1;
	int lowest_play_counter = INT_MAX;
	*voice_state = VOICE_ACTIVE;

	for (int i = 0; i < voice_count; i++)
	{
		if (voices[i].midi_channel == midi_channel)
		{
			if (voices[i].current_state == NOTE_NOT_PLAYING)
			{
				candidate_voice = i;
				*voice_state = VOICE_IDLE;
				break;
			}
			else
			{
				if (voices[i].play_counter < lowest_play_counter)
				{
					lowest_play_counter = voices[i].play_counter;
					candidate_voice = i;
				}
			}
		}
	}

	if (candidate_voice >= 0)
	{
		return voices + candidate_voice;
	}
	else
	{
		return NULL;
	}
}

voice_t *voice_find_playing_note(voice_t *voices, int voice_count, int midi_channel, int midi_note)
{
	for (int i = 0; i < voice_count; i++)
	{
		if (voices[i].current_state == midi_note && voices[i].midi_channel == midi_channel)
		{
			return voices + i;
		}
	}

	return NULL;
}
