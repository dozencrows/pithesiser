/*
 * setting.c
 *
 *  Created on: 14 Sep 2013
 *      Author: ntuckett
 */

#include "system_constants.h"
#include "setting.h"
#include <string.h>
#include <assert.h>

static void setting_set_name(setting_t* setting, const char* name)
{
	strncpy(setting->name, name, SETTING_MAX_NAME_LEN);
	setting->name[SETTING_MAX_NAME_LEN - 1] = 0;
}

void setting_init_as_int(setting_t* setting, const char* name, int value)
{
	setting->type = SETTING_TYPE_INT;
	setting_set_name(setting, name);
	setting->value.integer = value;
}

void setting_init_as_float(setting_t* setting, const char* name, float value)
{
	setting->type = SETTING_TYPE_FLOAT;
	setting_set_name(setting, name);
	setting->value.real = value;
}

void setting_init_as_enum(setting_t* setting, const char* name, int value, enum_type_info_t* enum_type)
{
	assert(value >= 0 && value < enum_type->max_index);
	setting->type = SETTING_TYPE_ENUM;
	setting_set_name(setting, name);
	setting->value.integer = value;
	setting->type_info.enum_type_info = enum_type;
}

void setting_set_value_int(setting_t* setting, int value)
{
	switch (setting->type)
	{
		case SETTING_TYPE_INT:
			setting->value.integer = value;
			break;

		case SETTING_TYPE_FLOAT:
			setting_set_value_float(setting, value);
			break;

		case SETTING_TYPE_ENUM:
			setting_set_value_enum_as_int(setting, value);
			break;
	}
}

void setting_set_value_float(setting_t* setting, float value)
{
	assert(setting->type == SETTING_TYPE_FLOAT);
	setting->value.real = value;
}

void setting_set_value_enum_as_int(setting_t* setting, int value)
{
	assert(setting->type == SETTING_TYPE_ENUM);
	assert(value >= 0 && value <= setting->type_info.enum_type_info->max_index);
	setting->value.integer = value;
}

int setting_get_value_int(setting_t* setting)
{
	assert(setting->type == SETTING_TYPE_INT);
	return setting->value.integer;
}

float setting_get_value_float(setting_t* setting)
{
	assert(setting->type == SETTING_TYPE_FLOAT);
	return setting->value.real;
}

const char* setting_get_value_enum(setting_t* setting)
{
	assert(setting->type == SETTING_TYPE_ENUM);
	return setting->type_info.enum_type_info->values[setting->value.integer];
}

int setting_get_value_enum_as_int(setting_t* setting)
{
	assert(setting->type == SETTING_TYPE_ENUM);
	return setting->value.integer;
}
