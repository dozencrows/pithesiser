/*
 * lfo.c
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#include "lfo.h"
#include <math.h>
#include "fixed_point_math.h"

void lfo_init(lfo_t *lfo)
{
	osc_init(&lfo->oscillator);
	lfo->oscillator.waveform = LFO_PROCEDURAL_SINE;
	lfo->oscillator.frequency = 1 * FIXED_ONE;
	lfo->oscillator.level = SHRT_MAX;
	lfo->value = 0;
}

void lfo_reset(lfo_t *lfo)
{
	lfo->oscillator.phase_accumulator = 0;
}

void lfo_update(lfo_t *lfo, int buffer_samples)
{
	osc_mid_output(&lfo->oscillator, &lfo->value, buffer_samples);
}
