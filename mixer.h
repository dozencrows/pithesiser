/*
 * mixer.h
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#ifndef MIXER_H_
#define MIXER_H_

#include "system_constants.h"

#define PAN_MAX		32768

extern void copy_mono_to_stereo(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest);
extern void mixdown_mono_to_stereo(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest);

extern void copy_mono_to_stereo_asm(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest);
extern void mixdown_mono_to_stereo_asm(sample_t *source, int32_t left, int32_t right, int sample_count, sample_t *dest);

#endif /* MIXER_H_ */
