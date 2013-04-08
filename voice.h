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

#define NOTE_ENDING				-2
#define NOTE_NOT_PLAYING		-1
#define PLAYING_NOTE(x)			(x >= 0)

#define VOICE_IDLE				0
#define VOICE_GONE_IDLE			1
#define VOICE_ACTIVE			2

typedef struct
{
	int	midi_channel;
	int last_note;
	int current_note;
	int play_counter;
	fixed_t frequency;
	envelope_instance_t	envelope_instance;
	oscillator_t oscillator;
} voice_t;

extern void voice_init(voice_t *voices, int voice_count, envelope_t *envelope);
extern int voice_update(voice_t *voice, int32_t master_level, sample_t *voice_buffer, int buffer_samples, int32_t timestep_ms, lfo_t *lfo);
extern void voice_play_note(voice_t *voice, int midi_note, waveform_type_t waveform);
extern void voice_stop_note(voice_t *voice);

#endif /* VOICE_H_ */
