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

extern int midi_controller_parse_index_controls(config_setting_t *config, int channel);
extern int midi_controller_parse_config(config_setting_t *config, midi_controller_t *controller);
extern void midi_controller_parser_report_error(config_setting_t *setting, const char* message, ...);

#endif /* MIDI_CONTROLLER_PARSER_H_ */
