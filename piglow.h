/*
 * piglow.h
 *
 *  Created on: 23 Dec 2013
 *      Author: ntuckett
 */

#ifndef PIGLOW_H_
#define PIGLOW_H_

typedef struct voice_t voice_t;
typedef struct config_setting_t config_setting_t;

extern void piglow_initialise(config_setting_t* config);
extern void piglow_update(voice_t* voice, int voice_count);
extern void piglow_deinitialise();

#endif /* PIGLOW_H_ */
