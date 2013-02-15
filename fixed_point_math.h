/*
 * fixed_point_math.h
 *
 *  Created on: 3 Feb 2013
 *      Author: ntuckett
 */

#ifndef FIXED_POINT_MATH_H_
#define FIXED_POINT_MATH_H_

#include "system_constants.h"

#define DOUBLE_TO_FIXED(x) (fixed_t)(x * (double)FIXED_ONE)

extern fixed_t const FIXED_PI;
extern fixed_t const FIXED_2_PI;
extern fixed_t const FIXED_HALF_PI;

extern void fixed_sin_cos(fixed_t const theta, fixed_t *s, fixed_t *c);

#define fixed_from_int(a) fixed_from_int_at(a, FIXED_PRECISION)
#define fixed_mul(a, b) fixed_mul_at(a, b, FIXED_PRECISION)
#define fixed_divide(a, b) fixed_divide_at(a, b, FIXED_PRECISION)
#define fixed_trunc_to_int(a) fixed_trunc_to_int_at(a, FIXED_PRECISION)
#define fixed_round_to_int(a) fixed_round_to_int_at(a, FIXED_PRECISION)

__attribute__((always_inline)) inline fixed_t fixed_from_int_at(int a, int precision)
{
	return (fixed_t)a << precision;
}

__attribute__((always_inline)) inline fixed_t fixed_mul_at(fixed_t a, fixed_t b, int precision)
{
	return ((fixed_wide_t)a * (fixed_wide_t)b) >> precision;
}

__attribute__((always_inline)) inline fixed_t fixed_divide_at(fixed_t a, fixed_t b, int precision)
{
	return ((fixed_wide_t)a << precision) / (fixed_wide_t)b;
}

__attribute__((always_inline)) inline int fixed_trunc_to_int_at(fixed_t a, int precision)
{
	return a >> precision;
}

__attribute__((always_inline)) inline int fixed_round_to_int_at(fixed_t a, int precision)
{
	return (a + (1 << (precision - 1))) >> precision;
}

#endif /* FIXED_POINT_MATH_H_ */
