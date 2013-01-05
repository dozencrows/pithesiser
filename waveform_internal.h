/*
 * waveform_internal.h
 *
 *  Created on: 2 Nov 2012
 *      Author: ntuckett
 */

#ifndef WAVEFORM_INTERNAL_H_
#define WAVEFORM_INTERNAL_H_

#define SYSTEM_SAMPLE_RATE			44100

#define FIXED_PRECISION				18
#define FIXED_FRACTION_MASK			0x0002ffff
#define FIXED_ONE					0x00040000

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

#define SCALE_AMPLITUDE(osc, sample)				sample = (sample * osc->level) / SHRT_MAX;

#define MIX(original, sample, mixed)				int32_t mixed = original + sample;	\
													mixed = (mixed * 75) / 100;			\
													if (mixed < -SHRT_MAX)				\
														mixed = -SHRT_MAX;				\
													else if (mixed > SHRT_MAX)			\
														mixed = SHRT_MAX;

#define STORE_SAMPLE(sample, sample_ptr)			*sample_ptr++ = (int16_t)sample; *sample_ptr++ = (int16_t)sample

#endif /* WAVEFORM_INTERNAL_H_ */
