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
