/*
 * waveform_internal.h
 *
 *  Created on: 2 Nov 2012
 *      Author: ntuckett
 */

#ifndef WAVEFORM_INTERNAL_H_
#define WAVEFORM_INTERNAL_H_

#include <sys/types.h>

#define GENFLAG_NONE				0x00000000
#define GENFLAG_LINEAR_INTERP		0x00000001

typedef struct
{
	void		*waveform_data;
	u_int32_t	flags;
} waveform_generator_def_t;

typedef struct oscillator_t oscillator_t;
typedef void (*generator_output_func_t)(waveform_generator_def_t *generator_def, oscillator_t* osc, sample_t *sample_data, int sample_count);

typedef struct
{
	waveform_generator_def_t	definition;
	generator_output_func_t		output_func;
	generator_output_func_t		mix_func;
	generator_output_func_t		mid_func;
} waveform_generator_t;

extern waveform_generator_t generators[];

#define AMPL_INTERP_PRECISION	15

#define CALC_AMPLITUDE_INTERPOLATION(osc, amp_scale, amp_delta, sample_count)	\
													int32_t amp_delta = ((osc->level - osc->last_level) << AMPL_INTERP_PRECISION) / sample_count; \
													int32_t amp_scale = osc->last_level << AMPL_INTERP_PRECISION;

#define INTERPOLATE_AMPLITUDE(amp_scale, amp_delta)	amp_scale += amp_delta;

#define SCALE_AMPLITUDE(amp_scale, sample)			sample = (sample * amp_scale) / SAMPLE_MAX;

#define MIX(original, sample, mixed)				int32_t mixed = original + sample;	\
													if (mixed < -SAMPLE_MAX)				\
														mixed = -SAMPLE_MAX;				\
													else if (mixed > SAMPLE_MAX)			\
														mixed = SAMPLE_MAX;

#define STORE_SAMPLE(sample, sample_ptr)			*sample_ptr++ = (sample_t)sample;

#endif /* WAVEFORM_INTERNAL_H_ */
