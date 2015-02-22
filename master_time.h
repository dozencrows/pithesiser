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
 * master_time.h
 *
 *  Created on: 6 Dec 2012
 *      Author: ntuckett
 */

#ifndef MASTER_TIME_H_
#define MASTER_TIME_H_

#include <stdint.h>

extern int32_t get_elapsed_time_ms();
extern int32_t get_elapsed_cpu_time_ns();

#endif /* MASTER_TIME_H_ */
