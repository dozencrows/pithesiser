/*
 * synth_controllers.c
 *
 *  Created on: 11 Feb 2013
 *      Author: ntuckett
 */

#include "synth_controllers.h"
#include "system_constants.h"
#include "midi.h"
#include "envelope.h"
#include "waveform.h"
#include "filter.h"

#define LFO_MIN_FREQUENCY		(FIXED_ONE / 10)
#define LFO_MAX_FREQUENCY		(20 * FIXED_ONE)
#define LFO_STATE_LAST			LFO_STATE_PITCH

#define FILTER_STATE_OFF		FILTER_PASS
#define FILTER_STATE_LAST		FILTER_HPF
#define FILTER_MIN_FREQUENCY	(FIXED_ONE / 10)
#define FILTER_MAX_FREQUENCY	(4186 * FIXED_ONE)
#define FILTER_MIN_Q			(FIXED_ONE / 100)
#define FILTER_MAX_Q			(FIXED_ONE)

#define WAVE_CONTROLLER			0x21
#define OSCILLOSCOPE_CONTROLLER	0x0e
#define VOLUME_CONTROLLER		0x02

#define ENVELOPE_ATTACK_LEVEL_CTRL	0x03	// slider 2
#define ENVELOPE_ATTACK_TIME_CTRL	0x0f	// dial 2
#define ENVELOPE_DECAY_LEVEL_CTRL	0x04	// slider 3
#define ENVELOPE_DECAY_TIME_CTRL	0x10	// dial 3
#define ENVELOPE_SUSTAIN_TIME_CTRL	0x11	// dial 4
#define ENVELOPE_RELEASE_TIME_CTRL	0x05	// slider 4

#define LFO_TOGGLE					0x1b	// upper switch 5
#define LFO_WAVEFORM				0x25	// lower switch 5
#define LFO_FREQUENCY				0x12	// dial 5
#define LFO_LEVEL					0x06	// slider 5

#define ENVELOPE_ATTACK_DURATION_SCALE	10
#define ENVELOPE_DECAY_DURATION_SCALE	10
#define ENVELOPE_SUSTAIN_DURATION_SCALE	10
#define ENVELOPE_RELEASE_DURATION_SCALE	10

#define FILTER_TOGGLE				0x1c	// upper switch 6
#define FILTER_FREQUENCY			0x13	// dial 6
#define FILTER_Q					0x08	// slider 6

midi_controller_t master_volume_controller;
midi_controller_t waveform_controller;
midi_controller_t oscilloscope_controller;

midi_controller_t envelope_attack_time_controller;
midi_controller_t envelope_attack_level_controller;
midi_controller_t envelope_decay_time_controller;
midi_controller_t envelope_decay_level_controller;
midi_controller_t envelope_sustain_time_controller;
midi_controller_t envelope_release_time_controller;

midi_controller_t lfo_state_controller;
midi_controller_t lfo_waveform_controller;
midi_controller_t lfo_level_controller;
midi_controller_t lfo_frequency_controller;

midi_controller_t filter_state_controller;
midi_controller_t filter_frequency_controller;
midi_controller_t filter_q_controller;

void synth_controllers_initialise(int controller_channel)
{
	midi_controller_create(&master_volume_controller);
	master_volume_controller.type = CONTINUOUS;
	master_volume_controller.midi_channel = controller_channel;
	master_volume_controller.midi_cc[0] = VOLUME_CONTROLLER;
	master_volume_controller.midi_range.min = 0;
	master_volume_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	master_volume_controller.output_min = 0;
	master_volume_controller.output_max = LEVEL_MAX;
	midi_controller_init(&master_volume_controller);

	midi_controller_create(&waveform_controller);
	waveform_controller.type = TOGGLE;
	waveform_controller.midi_channel = controller_channel;
	waveform_controller.midi_cc[0] = WAVE_CONTROLLER;
	waveform_controller.midi_threshold = MIDI_MID_CONTROLLER_VALUE;
	waveform_controller.output_min = WAVE_FIRST_AUDIBLE;
	waveform_controller.output_max = WAVE_LAST_AUDIBLE;
	midi_controller_init(&waveform_controller);

	midi_controller_create(&oscilloscope_controller);
	oscilloscope_controller.type = CONTINUOUS;
	oscilloscope_controller.midi_channel = controller_channel;
	oscilloscope_controller.midi_cc[0] = OSCILLOSCOPE_CONTROLLER;
	oscilloscope_controller.midi_range.min = 0;
	oscilloscope_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	oscilloscope_controller.output_min = 0;
	oscilloscope_controller.output_max = MIDI_MAX_CONTROLLER_VALUE;
	midi_controller_init(&oscilloscope_controller);

	midi_controller_create(&envelope_attack_time_controller);
	envelope_attack_time_controller.type = CONTINUOUS;
	envelope_attack_time_controller.midi_channel = controller_channel;
	envelope_attack_time_controller.midi_cc[0] = ENVELOPE_ATTACK_TIME_CTRL;
	envelope_attack_time_controller.midi_range.min = 0;
	envelope_attack_time_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	envelope_attack_time_controller.output_min = 0;
	envelope_attack_time_controller.output_max = ENVELOPE_ATTACK_DURATION_SCALE * MIDI_MAX_CONTROLLER_VALUE;
	midi_controller_init(&envelope_attack_time_controller);

	midi_controller_create(&envelope_attack_level_controller);
	envelope_attack_level_controller.type = CONTINUOUS;
	envelope_attack_level_controller.midi_channel = controller_channel;
	envelope_attack_level_controller.midi_cc[0] = ENVELOPE_ATTACK_LEVEL_CTRL;
	envelope_attack_level_controller.midi_range.min = 0;
	envelope_attack_level_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	envelope_attack_level_controller.output_min = 0;
	envelope_attack_level_controller.output_max = ENVELOPE_LEVEL_MAX;
	midi_controller_init(&envelope_attack_level_controller);

	midi_controller_create(&envelope_decay_time_controller);
	envelope_decay_time_controller.type = CONTINUOUS;
	envelope_decay_time_controller.midi_channel = controller_channel;
	envelope_decay_time_controller.midi_cc[0] = ENVELOPE_DECAY_TIME_CTRL;
	envelope_decay_time_controller.midi_range.min = 0;
	envelope_decay_time_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	envelope_decay_time_controller.output_min = 0;
	envelope_decay_time_controller.output_max = ENVELOPE_DECAY_DURATION_SCALE * MIDI_MAX_CONTROLLER_VALUE;
	midi_controller_init(&envelope_decay_time_controller);

	midi_controller_create(&envelope_decay_level_controller);
	envelope_decay_level_controller.type = CONTINUOUS;
	envelope_decay_level_controller.midi_channel = controller_channel;
	envelope_decay_level_controller.midi_cc[0] = ENVELOPE_DECAY_LEVEL_CTRL;
	envelope_decay_level_controller.midi_range.min = 0;
	envelope_decay_level_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	envelope_decay_level_controller.output_min = 0;
	envelope_decay_level_controller.output_max = ENVELOPE_LEVEL_MAX;
	midi_controller_init(&envelope_decay_level_controller);

	midi_controller_create(&envelope_sustain_time_controller);
	envelope_sustain_time_controller.type = CONTINUOUS_WITH_END;
	envelope_sustain_time_controller.midi_channel = controller_channel;
	envelope_sustain_time_controller.midi_cc[0] = ENVELOPE_SUSTAIN_TIME_CTRL;
	envelope_sustain_time_controller.midi_range.min = 0;
	envelope_sustain_time_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE - 1;
	envelope_sustain_time_controller.midi_range.end = DURATION_HELD;
	envelope_sustain_time_controller.output_min = 0;
	envelope_sustain_time_controller.output_max = ENVELOPE_SUSTAIN_DURATION_SCALE * MIDI_MAX_CONTROLLER_VALUE;
	midi_controller_init(&envelope_sustain_time_controller);

	midi_controller_create(&envelope_release_time_controller);
	envelope_release_time_controller.type = CONTINUOUS;
	envelope_release_time_controller.midi_channel = controller_channel;
	envelope_release_time_controller.midi_cc[0] = ENVELOPE_RELEASE_TIME_CTRL;
	envelope_release_time_controller.midi_range.min = 0;
	envelope_release_time_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	envelope_release_time_controller.output_min = 0;
	envelope_release_time_controller.output_max = ENVELOPE_RELEASE_DURATION_SCALE * MIDI_MAX_CONTROLLER_VALUE;
	midi_controller_init(&envelope_release_time_controller);

	midi_controller_create(&lfo_state_controller);
	lfo_state_controller.type = TOGGLE;
	lfo_state_controller.midi_channel = controller_channel;
	lfo_state_controller.midi_cc[0] = LFO_TOGGLE;
	lfo_state_controller.midi_threshold = MIDI_MID_CONTROLLER_VALUE;
	lfo_state_controller.output_min = LFO_STATE_OFF;
	lfo_state_controller.output_max = LFO_STATE_LAST;
	midi_controller_init(&lfo_state_controller);

	midi_controller_create(&lfo_waveform_controller);
	lfo_waveform_controller.type = TOGGLE;
	lfo_waveform_controller.midi_channel = controller_channel;
	lfo_waveform_controller.midi_cc[0] = LFO_WAVEFORM;
	lfo_waveform_controller.midi_threshold = MIDI_MID_CONTROLLER_VALUE;
	lfo_waveform_controller.output_min = WAVE_FIRST_LFO;
	lfo_waveform_controller.output_max = WAVE_LAST_LFO;
	midi_controller_init(&lfo_waveform_controller);

	midi_controller_create(&lfo_frequency_controller);
	lfo_frequency_controller.type = CONTINUOUS;
	lfo_frequency_controller.midi_channel = controller_channel;
	lfo_frequency_controller.midi_cc[0] = LFO_FREQUENCY;
	lfo_frequency_controller.midi_range.min = 0;
	lfo_frequency_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	lfo_frequency_controller.output_min = LFO_MIN_FREQUENCY;
	lfo_frequency_controller.output_max = LFO_MAX_FREQUENCY;
	midi_controller_init(&lfo_frequency_controller);

	midi_controller_create(&lfo_level_controller);
	lfo_level_controller.type = CONTINUOUS;
	lfo_level_controller.midi_channel = controller_channel;
	lfo_level_controller.midi_cc[0] = LFO_LEVEL;
	lfo_level_controller.midi_range.min = 0;
	lfo_level_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	lfo_level_controller.output_min = 0;
	lfo_level_controller.output_max = LEVEL_MAX;
	midi_controller_init(&lfo_level_controller);

	midi_controller_create(&filter_state_controller);
	filter_state_controller.type = TOGGLE;
	filter_state_controller.midi_channel = controller_channel;
	filter_state_controller.midi_cc[0] = FILTER_TOGGLE;
	filter_state_controller.midi_threshold = MIDI_MID_CONTROLLER_VALUE;
	filter_state_controller.output_min = FILTER_STATE_OFF;
	filter_state_controller.output_max = FILTER_STATE_LAST;
	midi_controller_init(&filter_state_controller);

	midi_controller_create(&filter_frequency_controller);
	filter_frequency_controller.type = CONTINUOUS;
	filter_frequency_controller.midi_channel = controller_channel;
	filter_frequency_controller.midi_cc[0] = FILTER_FREQUENCY;
	filter_frequency_controller.midi_range.min = 0;
	filter_frequency_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	filter_frequency_controller.output_min = FILTER_MIN_FREQUENCY;
	filter_frequency_controller.output_max = FILTER_MAX_FREQUENCY;
	midi_controller_init(&filter_frequency_controller);

	midi_controller_create(&filter_q_controller);
	filter_q_controller.type = CONTINUOUS;
	filter_q_controller.midi_channel = controller_channel;
	filter_q_controller.midi_cc[0] = FILTER_Q;
	filter_q_controller.midi_range.min = 0;
	filter_q_controller.midi_range.max = MIDI_MAX_CONTROLLER_VALUE;
	filter_q_controller.output_min = FILTER_MIN_Q;
	filter_q_controller.output_max = FILTER_MAX_Q;
	midi_controller_init(&filter_q_controller);
}
