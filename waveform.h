/*
 * waveform.h
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#ifndef WAVEFORM_H_
#define WAVEFORM_H_

#include <sys/types.h>

typedef enum
{
	WAVE_FIRST = 0,

	WAVETABLE_SINE = 0,
	WAVETABLE_SAW,
	WAVETABLE_SINE_LINEAR,
	WAVETABLE_SAW_LINEAR,

	WAVE_LAST = WAVETABLE_SAW_LINEAR,
	WAVE_COUNT
} waveform_type_t;

#define FIXED_PRECISION				16
#define FIXED_FRACTION_MASK			0x0000ffff

#define GENFLAG_NONE				0x00000000
#define GENFLAG_LINEAR_INTERP		0x00000001

typedef struct
{
	void		*waveform_data;
	u_int32_t	flags;
} waveform_generator_def_t;

typedef struct oscillator_t oscillator_t;
typedef void (*generator_output_func_t)(waveform_generator_def_t *generator_def, oscillator_t* osc, int sample_count, void *sample_data);

typedef struct
{
	waveform_generator_def_t	definition;
	generator_output_func_t		output_func;
	generator_output_func_t		mix_func;
} waveform_generator_t;

extern waveform_generator_t generators[];

extern void waveform_initialise();

#endif /* WAVEFORM_H_ */
