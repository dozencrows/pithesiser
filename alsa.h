/*
 * alsa.h
 *
 *  Created on: 29 Oct 2012
 *      Author: ntuckett
 */

#ifndef ALSA_H_
#define ALSA_H_

extern int alsa_initialise(const char* device_name);
extern void alsa_deinitialise();

#endif /* ALSA_H_ */
