/*
 * waveform_wavetable.h
 *
 *  Created on: 1 Nov 2012
 *      Author: ntuckett
 */

#ifndef WAVEFORM_WAVETABLE_H_
#define WAVEFORM_WAVETABLE_H_

#include "waveform.h"
#include "waveform_internal.h"

extern void init_wavetables();
extern void init_wavetable_generator(waveform_type_t waveform_type, waveform_generator_t *generator);

#endif /* WAVEFORM_WAVETABLE_H_ */
