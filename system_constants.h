/*
 * system_constants.h
 *
 *  Created on: 12 Jan 2013
 *      Author: ntuckett
 */

#ifndef SYSTEM_CONSTANTS_H_
#define SYSTEM_CONSTANTS_H_

#include <sys/types.h>

#define SYSTEM_SAMPLE_RATE			44100
#define SYSTEM_FRAME_RATE			60

#define FIXED_PRECISION				18
#define FIXED_FRACTION_MASK			0x0002ffff
#define FIXED_ONE					0x00040000
#define FIXED_HALF					0x00020000

typedef int16_t	sample_t;
typedef int32_t	fixed_t;
typedef int64_t	fixed_wide_t;

#define SAMPLE_MAX			SHRT_MAX
#define BYTES_PER_CHANNEL	(sizeof(sample_t))
#define CHANNELS_PER_SAMPLE	2
#define BYTES_PER_SAMPLE	(BYTES_PER_CHANNEL * CHANNELS_PER_SAMPLE)

#endif /* SYSTEM_CONSTANTS_H_ */
