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
	SETTING_TYPE_INT,
	SETTING_TYPE_FLOAT,
	SETTING_TYPE_ENUM
} setting_type_t;

typedef struct enum_type_info_t
{
	int	max_index;
	const char** values;
} enum_type_info_t;

typedef struct setting_type_info_t
{
	union
	{

	};
} setting_type_info_t;

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

extern void setting_init_as_int(setting_t* setting, const char* name, int value);
extern void setting_init_as_float(setting_t* setting, const char* name, float value);
extern void setting_init_as_enum(setting_t* setting, const char* name, int value, enum_type_info_t* enum_type);

extern void setting_set_value_int(setting_t* setting, int value);
extern void setting_set_value_float(setting_t* setting, float value);
extern void setting_set_value_enum_as_int(setting_t* setting, int value);

extern int setting_get_value_int(setting_t* setting);
extern float setting_get_value_float(setting_t* setting);
extern const char* setting_get_value_enum(setting_t* setting);
extern int setting_get_value_enum_as_int(setting_t* setting);

#endif /* SETTING_H_ */
