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

#endif /* FIXED_POINT_MATH_H_ */
