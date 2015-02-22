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
 * gfx_object.h
 *
 *  Created on: 19 Jan 2013
 *      Author: ntuckett
 */

#ifndef GFX_OBJECT_H_
#define GFX_OBJECT_H_

typedef unsigned int object_id_t;

typedef struct gfx_object_t
{
	object_id_t	id;
} gfx_object_t;

#define GFX_ANY_OBJECT	-1

#endif /* GFX_OBJECT_H_ */
