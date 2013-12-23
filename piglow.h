/*
 * piglow.h
 *
 *  Created on: 23 Dec 2013
 *      Author: ntuckett
 */

#ifndef PIGLOW_H_
#define PIGLOW_H_

typedef struct voice_t;

extern void piglow_initialise();
extern void piglow_update(voice_t* voice, int voice_count);

#endif /* PIGLOW_H_ */
