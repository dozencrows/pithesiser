/*
 * voice.c
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#include "voice.h"
#include <stddef.h>
#include "midi.h"
#include "logging.h"

#define MAX_VOICE_CALLBACKS		4

typedef struct voice_callback_info_t voice_callback_info_t;
typedef struct voice_callback_info_t
{
	voice_callback_t		callback;
	void*					data;
	voice_callback_info_t*	next_callback;
} voice_callback_info_t;

static voice_callback_info_t* voice_callback_head = NULL;
static voice_callback_info_t* voice_callback_free = NULL;
static voice_callback_info_t voice_callback_info[MAX_VOICE_CALLBACKS];

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

void voices_initialise(voice_t *voices, int voice_count, envelope_t *level_envelope, envelope_t *filter_freq_envelope, envelope_t *filter_q_envelope)
{
	for (int i = 0; i < voice_count; i++)
	{
		init_voice(voices + i, level_envelope, filter_freq_envelope, filter_q_envelope);
	}

	for (int i = 0; i < MAX_VOICE_CALLBACKS; i++)
	{
		voice_callback_info[i].next_callback = voice_callback_free;
		voice_callback_free = voice_callback_info + i;
	}
}

void voices_add_callback(voice_callback_t callback, void* callback_data)
{
	if (voice_callback_free != NULL)
	{
		voice_callback_info_t* callback_info = voice_callback_free;
		voice_callback_free = voice_callback_free->next_callback;

		callback_info->callback 		= callback;
		callback_info->data 			= callback_data;
		callback_info->next_callback 	= voice_callback_head;

		voice_callback_head = callback_info;
	}
	else
	{
		LOG_ERROR("No voice callbacks available");
	}
}

void voices_remove_callback(voice_callback_t callback)
{
	voice_callback_info_t* callback_info = voice_callback_head;
	voice_callback_info_t** last_link = &voice_callback_head;

	while (callback_info != NULL)
	{
		if (callback_info->callback == callback)
		{
			*last_link = callback_info->next_callback;
			callback_info->callback 		= NULL;
			callback_info->data				= NULL;
			callback_info->next_callback 	= voice_callback_free;

			voice_callback_free = callback_info;
			break;
		}
		else
		{
			last_link = &callback_info->next_callback;
			callback_info = callback_info->next_callback;
		}
	}
}

void voice_make_callback(voice_event_t event, voice_t* voice)
{
	voice_callback_info_t* callback_info = voice_callback_head;

	while (callback_info != NULL)
	{
		callback_info->callback(event, voice, callback_info->data);
		callback_info = callback_info->next_callback;
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
			voice->oscillator.phase_accumulator = 0;
			voice_make_callback(VOICE_EVENT_NOTE_STARTING, voice);
			//voice->oscillator.frequency = voice->frequency;
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
		else if (voice->current_state == NOTE_ENDING)
		{
			voice->current_state = NOTE_NOT_PLAYING;
			voice_make_callback(VOICE_EVENT_NOTE_ENDED, voice);
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
		voice_make_callback(VOICE_EVENT_NOTE_ENDING, voice);
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
