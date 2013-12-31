/*
 * modulation_matrix_controller.h
 *
 *  Created on: 31 Dec 2013
 *      Author: ntuckett
 */

#ifndef MODULATION_MATRIX_CONTROLLER_H_
#define MODULATION_MATRIX_CONTROLLER_H_

typedef struct config_setting_t config_setting_t;

extern int mod_matrix_controller_initialise(config_setting_t *config);
extern void mod_matrix_controller_process_midi(int channel, unsigned char event_type, unsigned char data0, unsigned char data1);

#endif /* MODULATION_MATRIX_CONTROLLER_H_ */
