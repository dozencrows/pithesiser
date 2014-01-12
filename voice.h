/*
 * voice.h
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#ifndef VOICE_H_
#define VOICE_H_

#include "envelope.h"
#include "oscillator.h"
#include "lfo.h"
#include "filter.h"

#define NOTE_ENDING				-2
#define NOTE_NOT_PLAYING		-1
#define PLAYING_NOTE(x)			(x >= 0)

#define VOICE_IDLE				0
#define VOICE_GONE_IDLE			1
#define VOICE_ACTIVE			2

#define VOICE_FLAG_
typedef struct voice_t
{
	int index;
	int	midi_channel;
	int note;
	int last_state;
	int current_state;
	int play_counter;
	fixed_t frequency;
	filter_definition_t filter_def;
	oscillator_t oscillator;
	filter_t filter;
} voice_t;

typedef enum voice_event_t
{
	VOICE_EVENT_NOTE_STARTING,
	VOICE_EVENT_NOTE_ENDING,
	VOICE_EVENT_NOTE_ENDED
} voice_event_t;

typedef void (*voice_callback_t)(voice_event_t callback_event, voice_t* voice, void* callback_data);

extern void voices_initialise(voice_t *voices, int voice_count);
extern void voices_add_callback(voice_callback_t callback, void* callback_data);
extern void voices_remove_callback(voice_callback_t callback);

extern void voice_preupdate(voice_t *voice, int32_t timestep_ms, filter_definition_t *filter_def);
extern int voice_update(voice_t *voice, int32_t master_level, sample_t *voice_buffer, int buffer_samples, int32_t timestep_ms, filter_definition_t *filter_def);
extern void voice_play_note(voice_t *voice, int midi_note, waveform_type_t waveform);
extern void voice_stop_note(voice_t *voice);
extern voice_t *voice_find_next_likely_free(voice_t *voices, int voice_count, int midi_channel, int *voice_state);
extern voice_t *voice_find_playing_note(voice_t *voices, int voice_count, int midi_channel, int midi_note);

#endif /* VOICE_H_ */
