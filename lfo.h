/*
 * lfo.h
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#ifndef LFO_H_
#define LFO_H_

#include "oscillator.h"

#define LFO_STATE_OFF			0
#define LFO_STATE_VOLUME		1
#define LFO_STATE_PITCH			2

typedef struct
{
	int	state;
	oscillator_t oscillator;
	sample_t value;
} lfo_t;

extern void lfo_init(lfo_t *lfo);
extern void lfo_reset(lfo_t *lfo);
extern void lfo_update(lfo_t *lfo, int buffer_samples);
extern void lfo_modulate_oscillator(lfo_t *lfo, oscillator_t *osc);

#endif /* LFO_H_ */
