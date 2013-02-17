/*
 * filter.h
 *
 *  Created on: 3 Feb 2013
 *      Author: ntuckett
 */

#ifndef FILTER_H_
#define FILTER_H_

#include "system_constants.h"

#define FILTER_PASS	0
#define FILTER_LPF	1
#define FILTER_HPF	2

#define FILTER_FIXED_PRECISION	14
#define FILTER_FIXED_ONE		(1 << FILTER_FIXED_PRECISION)

typedef struct filter_definition_t
{
	int		type;
	fixed_t	frequency;
	fixed_t	gain;
	fixed_t q;
} filter_definition_t;

typedef struct filter_state_t
{
	fixed_t input_coeff[3];
	fixed_t output_coeff[2];
	fixed_t history[2];
	fixed_t output[2];
} filter_state_t;

typedef struct filter_t
{
	filter_definition_t	definition;
	filter_state_t		state;
} filter_t;

extern void filter_init(filter_t *filter);
extern void filter_update(filter_t *filter);
extern void filter_apply(filter_t *filter, sample_t *sample_data, int sample_count);

#endif /* FILTER_H_ */
