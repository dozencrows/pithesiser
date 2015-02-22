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
