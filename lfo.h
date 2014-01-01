/*
 * lfo.h
 *
 *  Created on: 8 Apr 2013
 *      Author: ntuckett
 */

#ifndef LFO_H_
#define LFO_H_

#include "oscillator.h"
#include "modulation_matrix.h"

typedef struct lfo_t
{
	mod_matrix_source_t	mod_matrix_source;
	oscillator_t 		oscillator;
	sample_t 			value;
} lfo_t;

extern void lfo_init(lfo_t *lfo, const char* name);
extern void lfo_reset(lfo_t *lfo);
extern void lfo_update(lfo_t *lfo, int buffer_samples);

#endif /* LFO_H_ */
