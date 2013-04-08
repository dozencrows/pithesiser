/*
 * voice.c
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#include "voice.h"
#include <math.h>
#include "midi.h"
#include "synth_controllers.h"
#include "fixed_point_math.h"

static void init_voice(voice_t *voice, envelope_t *envelope)
{
	voice->midi_channel = 0;
	voice->last_note = NOTE_NOT_PLAYING;
	voice->current_note = NOTE_NOT_PLAYING;
	voice->play_counter = 0;

	osc_init(&voice->oscillator);
	envelope_init(&voice->envelope_instance, envelope);
}

void voice_init_voices(voice_t *voices, int voice_count, envelope_t *envelope)
{
	for (int i = 0; i < voice_count; i++)
	{
		init_voice(voices + i, envelope);
	}
}

// TODO Encapsulate LFO data & functionality
int voice_update(voice_t *voice, int32_t master_level, sample_t *voice_buffer, int buffer_samples, int32_t timestep_ms, sample_t lfo_value, int lfo_state)
{
	int voice_state = VOICE_IDLE;

	if (voice->current_note != voice->last_note)
	{
		if (voice->current_note == NOTE_NOT_PLAYING)
		{
			voice->oscillator.level = 0;
		}
		else if (voice->current_note >= 0)
		{
			voice->oscillator.frequency = voice->frequency;
			voice->oscillator.phase_accumulator = 0;
			envelope_start(&voice->envelope_instance);
		}

		if (voice->last_note == NOTE_NOT_PLAYING)
		{
			voice->oscillator.last_level = -1;
		}

		voice->last_note = voice->current_note;
	}

	if (voice->current_note != NOTE_NOT_PLAYING)
	{
		int32_t envelope_level = envelope_step(&voice->envelope_instance, timestep_ms);
		int32_t note_level = (LEVEL_MAX * envelope_level) / ENVELOPE_LEVEL_MAX;

		fixed_t osc_frequency = voice->frequency;

		if (lfo_state == LFO_STATE_VOLUME)
		{
			if (lfo_value > 0)
			{
				note_level = (note_level * lfo_value) / SHRT_MAX;
			}
			else
			{
				note_level = 0;
			}
		}
		else if (lfo_state == LFO_STATE_PITCH)
		{
			// TODO: use a proper fixed point power function!
			osc_frequency = fixed_mul(osc_frequency, powf(2.0f, (float)lfo_value / (float)SHRT_MAX) * FIXED_ONE);
		}

		voice->oscillator.frequency = osc_frequency;
		voice->oscillator.level = (note_level * master_level) / LEVEL_MAX;

		// If this is a new note from scratch, avoid interpolating the initial level across the chunk.
		if (voice->oscillator.last_level < 0)
		{
			voice->oscillator.last_level = voice->oscillator.level;
		}

		if (voice->oscillator.level > 0 || voice->oscillator.last_level != 0)
		{
			osc_output(&voice->oscillator, voice_buffer, buffer_samples);
			voice->oscillator.last_level = voice->oscillator.level;
			voice_state = VOICE_ACTIVE;
		}
		else if (voice->current_note == NOTE_ENDING || envelope_completed(&voice->envelope_instance))
		{
			voice->current_note = NOTE_NOT_PLAYING;
			voice_state = VOICE_GONE_IDLE;
		}
	}

	return voice_state;
}

void voice_play_note(voice_t *voice, int midi_note, waveform_type_t waveform)
{
	voice->current_note = midi_note;
	voice->oscillator.waveform = waveform;
	voice->frequency = midi_get_note_frequency(midi_note);
}

void voice_stop_note(voice_t *voice)
{
	if (PLAYING_NOTE(voice->current_note))
	{
		voice->current_note = NOTE_ENDING;
		envelope_go_to_stage(&voice->envelope_instance, ENVELOPE_STAGE_RELEASE);
	}
}
