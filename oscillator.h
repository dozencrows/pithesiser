/*
 * oscillator.h
 *
 *  Created on: 31 Oct 2012
 *      Author: ntuckett
 *
 *  Assumptions:
 *  	2 channels
 *  	Sample format is signed 16-bit little endian
 *  	Sample rate is 44100Hz
 */

#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include <sys/types.h>

typedef enum
{
	WAVE_FIRST = 0,

	WAVETABLE_SINE = 0,
	WAVETABLE_SAW,
	WAVETABLE_SINE_LINEAR,
	WAVETABLE_SAW_LINEAR,

	WAVE_LAST = WAVETABLE_SAW_LINEAR
} waveform_type_t;

typedef struct
{
	waveform_type_t		waveform;
	int					frequency;
	u_int32_t			phase_accumulator;
	int32_t				level;
} oscillator_t;

extern void osc_init(oscillator_t* osc);
extern void osc_output(oscillator_t* osc, int sample_count, void *sample_data);
extern void osc_mix_output(oscillator_t* osc, int sample_count, void *sample_data);

#endif /* OSCILLATOR_H_ */
