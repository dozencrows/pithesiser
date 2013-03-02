/*
 * recording.h
 *
 *  Created on: 2 Mar 2013
 *      Author: ntuckett
 */

#ifndef RECORDING_H_
#define RECORDING_H_

#include <libconfig.h>

extern void recording_initialise(config_t *config, int gfx_event_id);
extern void recording_deinitialise();


#endif /* RECORDING_H_ */
