/*
 * midi_controller_parser.h
 *
 *  Created on: 26 Dec 2013
 *      Author: ntuckett
 */

#ifndef MIDI_CONTROLLER_PARSER_H_
#define MIDI_CONTROLLER_PARSER_H_

typedef struct config_setting_t config_setting_t;
typedef struct midi_controller_t midi_controller_t;

typedef struct controller_parser_t
{
	const char *config_setting_name;
	midi_controller_t *controller;
	void(*set_output)(midi_controller_t *controller);
} controller_parser_t;

extern int midi_controller_parse_config(config_setting_t *config, midi_controller_t *controller);
extern void midi_controller_parser_report_error(config_setting_t *setting, const char* message, ...);

#endif /* MIDI_CONTROLLER_PARSER_H_ */
