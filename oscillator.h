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
	WAVE_SINE,
	WAVE_SAW
} waveform_type_t;

typedef struct
{
	waveform_type_t		waveform;
	float				frequency;
	u_int32_t			phase_accumulator;
} oscillator_t;

extern void osc_init(oscillator_t* osc);
extern void osc_output(oscillator_t* osc, int sample_count, void *sample_data);

#endif /* OSCILLATOR_H_ */
