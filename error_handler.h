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
 * error_handler.h
 *
 *  Created on: 26 Dec 2013
 *      Author: ntuckett
 */

#ifndef ERROR_HANDLER_H_
#define ERROR_HANDLER_H_

typedef struct config_setting_t config_setting_t;

extern void push_last_error();
extern void push_custom_error(const char* message);
extern void pop_error_report(const char* message);
extern void setting_error_report(config_setting_t *setting, const char* message, ...);

#endif /* ERROR_HANDLER_H_ */
