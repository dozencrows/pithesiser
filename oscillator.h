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
#include "system_constants.h"
#include "waveform.h"

typedef struct oscillator_t
{
	waveform_type_t		waveform;
	fixed_t				frequency;
	fixed_t				phase_accumulator;
	int32_t				level;
} oscillator_t;

extern void osc_init(oscillator_t* osc);
extern void osc_output(oscillator_t* osc, sample_t *sample_data, int sample_count);
extern void osc_mix_output(oscillator_t* osc, sample_t *sample_data, int sample_count);
extern void osc_mid_output(oscillator_t* osc, sample_t *sample_data, int sample_count);

#endif /* OSCILLATOR_H_ */
