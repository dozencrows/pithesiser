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
 * logging.h
 *
 *  Created on: 5 Jan 2014
 *      Author: ntuckett
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <log4c.h>

extern int logging_initialise();

#if defined(LOGGING_ENABLED)

extern log4c_category_t* log_cat;

#define LOG_INFO(msg, ...) log4c_category_info(log_cat, msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...) log4c_category_warn(log_cat, msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) log4c_category_error(log_cat, msg, ##__VA_ARGS__)

#else

#define LOG_INFO(msg ...)
#define LOG_WARN(msg ...)
#define LOG_ERROR(msg ...)

#endif

#endif /* LOGGING_H_ */
