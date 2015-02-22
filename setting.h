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
 * setting.h
 *
 *  Created on: 14 Sep 2013
 *      Author: ntuckett
 */

#ifndef SETTING_H_
#define SETTING_H_

#define SETTING_MAX_NAME_LEN	128

typedef enum setting_type_t
{
	SETTING_TYPE_UNKNOWN,
	SETTING_TYPE_INT,
	SETTING_TYPE_FLOAT,
	SETTING_TYPE_ENUM
} setting_type_t;

typedef struct enum_type_info_t
{
	int	max_index;
	const char** values;
} enum_type_info_t;

typedef struct setting_t
{
	char 					name[SETTING_MAX_NAME_LEN];
	setting_type_t			type;
	union
	{
		enum_type_info_t*	enum_type_info;
	} type_info;
	union
	{
		int					integer;
		float				real;
	} value;
} setting_t;

extern setting_t* setting_create(const char* name);
extern void setting_destroy(setting_t* setting);
extern setting_t* setting_find(const char* name);
extern void setting_init_as_int(setting_t* setting, int value);
extern void setting_init_as_float(setting_t* setting, float value);
extern void setting_init_as_enum(setting_t* setting, int value, enum_type_info_t* enum_type);

extern void setting_set_value_int(setting_t* setting, int value);
extern void setting_set_value_float(setting_t* setting, float value);
extern void setting_set_value_enum_as_int(setting_t* setting, int value);

extern int setting_get_value_int(setting_t* setting);
extern float setting_get_value_float(setting_t* setting);
extern const char* setting_get_value_enum(setting_t* setting);
extern int setting_get_value_enum_as_int(setting_t* setting);

#endif /* SETTING_H_ */
