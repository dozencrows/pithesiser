// Pithesiser - a software synthesiser for Raspberry Pi
// Copyright (C) 2015 Nicholas Tuckett
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

/*
 * fixed_point_math.c
 *
 *  Created on: 3 Feb 2013
 *      Author: ntuckett
 *
 *      http://www.drdobbs.com/cpp/optimizing-math-intensive-applications-w/207000448?pgno=2
 */

#include "fixed_point_math.h"
#include <math.h>

fixed_t const FIXED_PI		= DOUBLE_TO_FIXED(M_PI);
fixed_t const FIXED_2_PI	= DOUBLE_TO_FIXED(M_PI * 2.0);
fixed_t const FIXED_HALF_PI	= DOUBLE_TO_FIXED(M_PI * 0.5);

fixed_t const CORDIC_SCALE	= DOUBLE_TO_FIXED(0.271572);

const fixed_wide_t arctantab[32] = {
	DOUBLE_TO_FIXED(1.107148718),
	DOUBLE_TO_FIXED(0.785398163),
	DOUBLE_TO_FIXED(0.463647608),
	DOUBLE_TO_FIXED(0.244978663),
	DOUBLE_TO_FIXED(0.124354996),
	DOUBLE_TO_FIXED(0.062418811),
	DOUBLE_TO_FIXED(0.031239834),
	DOUBLE_TO_FIXED(0.01562373),
	DOUBLE_TO_FIXED(0.00781234),
	DOUBLE_TO_FIXED(0.003906231),
	DOUBLE_TO_FIXED(0.001953121),
	DOUBLE_TO_FIXED(0.000976562),
	DOUBLE_TO_FIXED(0.000488281),
	DOUBLE_TO_FIXED(0.000244141),
	DOUBLE_TO_FIXED(0.00012207),
	DOUBLE_TO_FIXED(0.000061035),
	DOUBLE_TO_FIXED(0.000030517),
	DOUBLE_TO_FIXED(0.000015259),
	DOUBLE_TO_FIXED(0.000007629),
	DOUBLE_TO_FIXED(0.000003815),
	DOUBLE_TO_FIXED(0.000001907),
	DOUBLE_TO_FIXED(0.000000954),
	DOUBLE_TO_FIXED(0.000000477),
	DOUBLE_TO_FIXED(0.000000238),
	DOUBLE_TO_FIXED(0.000000119),
	DOUBLE_TO_FIXED(0.00000006),
	DOUBLE_TO_FIXED(0.00000003),
	DOUBLE_TO_FIXED(0.000000015),
	DOUBLE_TO_FIXED(0.000000007),
	DOUBLE_TO_FIXED(0.000000004),
	DOUBLE_TO_FIXED(0.000000002),
	DOUBLE_TO_FIXED(0.000000001)
};

static fixed_wide_t scale_cordic_result(long a)
{
    return (long)((((fixed_wide_t)a)*CORDIC_SCALE)>>FIXED_PRECISION);
}

static fixed_wide_t right_shift(fixed_wide_t val, int shift)
{
    return (shift<0)?(val<<-shift):(val>>shift);
}

static void cordic_rotation(fixed_wide_t *px, fixed_wide_t *py, long theta)
{
	fixed_wide_t x = *px, y = *py;
	fixed_wide_t const *arctanptr = arctantab;
    for (int i = -1; i <= (int)FIXED_PRECISION; ++i)
    {
    	fixed_wide_t const yshift=right_shift(y,i);
    	fixed_wide_t const xshift=right_shift(x,i);

        if (theta < 0)
        {
            x += yshift;
            y -= xshift;
            theta += *arctanptr++;
        }
        else
        {
            x -= yshift;
            y += xshift;
            theta -= *arctanptr++;
        }
    }
    *px = scale_cordic_result(x);
    *py = scale_cordic_result(y);
}

void fixed_sin_cos(fixed_t const theta, fixed_t *s, fixed_t *c)
{
    fixed_wide_t x = theta % FIXED_2_PI;
    if (x < 0)
        x += FIXED_2_PI;

    int negate_cos = 0;
    int negate_sin = 0;

    if (x > FIXED_PI)
    {
        x = FIXED_2_PI - x;
        negate_sin = 1;
    }
    if (x > FIXED_HALF_PI)
    {
        x = FIXED_PI - x;
        negate_cos = 1;
    }

    fixed_wide_t x_cos = FIXED_ONE, x_sin = 0;

    cordic_rotation(&x_cos, &x_sin, x);

    if (s)
    {
        *s = negate_sin ? -x_sin : x_sin;
    }
    if (c)
    {
        *c = negate_cos ? -x_cos : x_cos;
    }
}
