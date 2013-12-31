/*
 * lfo.c
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#include "lfo.h"
#include <math.h>
#include "fixed_point_math.h"

mod_matrix_value_t lfo_generate_value(mod_matrix_source_t* source)
{
	lfo_t* lfo = (lfo_t*)source;
	return (mod_matrix_value_t) lfo->value;
}

void lfo_init(lfo_t *lfo, const char* name)
{
	mod_matrix_init_source(name, lfo_generate_value, &lfo->mod_matrix_source);
	mod_matrix_add_source(&lfo->mod_matrix_source);
	osc_init(&lfo->oscillator);
	lfo->oscillator.waveform = LFO_PROCEDURAL_SINE;
	lfo->oscillator.frequency = 1 * FIXED_ONE;
	lfo->oscillator.level = SHRT_MAX;
	lfo->state = LFO_STATE_OFF;
	lfo->value = 0;
}

void lfo_reset(lfo_t *lfo)
{
	lfo->oscillator.phase_accumulator = 0;
}

void lfo_update(lfo_t *lfo, int buffer_samples)
{
	if (lfo->state)
	{
		osc_mid_output(&lfo->oscillator, &lfo->value, buffer_samples);
	}
	else
	{
		lfo->value = 0;
	}
}

void lfo_modulate_oscillator(lfo_t *lfo, oscillator_t *osc)
{
	if (lfo->state == LFO_STATE_VOLUME)
	{
		if (lfo->value > 0)
		{
			osc->level = (osc->level * lfo->value) / SHRT_MAX;
		}
		else
		{
			osc->level = 0;
		}
	}
	else if (lfo->state == LFO_STATE_PITCH)
	{
		// TODO: use a proper fixed point power function!
		osc->frequency = fixed_mul(osc->frequency, powf(2.0f, (float)lfo->value / (float)SHRT_MAX) * FIXED_ONE);
	}
}
