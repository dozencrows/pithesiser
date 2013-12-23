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

typedef struct
{
	int	midi_channel;
	int note;
	int last_state;
	int current_state;
	int play_counter;
	fixed_t frequency;
	filter_definition_t filter_def;
	envelope_instance_t	level_envelope_instance;
	envelope_instance_t filter_freq_envelope_instance;
	envelope_instance_t filter_q_envelope_instance;
	oscillator_t oscillator;
	filter_t filter;
} voice_t;

extern void voice_init(voice_t *voices, int voice_count, envelope_t *level_envelope, envelope_t *filter_freq_envelope, envelope_t *filter_q_envelope);
extern int voice_update(voice_t *voice, int32_t master_level, sample_t *voice_buffer, int buffer_samples, int32_t timestep_ms, lfo_t *lfo, filter_definition_t *filter_def);
extern void voice_play_note(voice_t *voice, int midi_note, waveform_type_t waveform);
extern void voice_stop_note(voice_t *voice);
extern voice_t *voice_find_next_likely_free(voice_t *voices, int voice_count, int *voice_state);
extern voice_t *voice_find_playing_note(voice_t *voices, int voice_count, int midi_note);

#endif /* VOICE_H_ */
