/*
 * filter_float.h
 *
 *  Created on: 15 Feb 2013
 *      Author: ntuckett
 */

#ifndef FILTER_FLOAT_H_
#define FILTER_FLOAT_H_

typedef struct float_filter_definition_t
{
	int		type;
	float	frequency;
	float	gain;
	float q;
} float_filter_definition_t;

typedef struct float_filter_state_t
{
	float input_coeff[3];
	float output_coeff[2];
	float history[2];
	float output[2];
} float_filter_state_t;

typedef struct float_filter_t
{
	float_filter_definition_t	definition;
	float_filter_state_t		state;
} float_filter_t;

extern void float_filter_init(float_filter_t *filter);
extern void float_filter_update(float_filter_t *filter);
extern void float_filter_apply(float_filter_t *filter, float *sample_data, int sample_count);

#endif /* FILTER_FLOAT_H_ */
