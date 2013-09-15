/*
 * setting.c
 *
 *  Created on: 14 Sep 2013
 *      Author: ntuckett
 */

#include "system_constants.h"
#include "setting.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

//------------------------------------------------------------------------------
// Internal link list structure for keeping track of settings
//
typedef struct link_double_t link_double_t;

struct link_double_t
{
	link_double_t* next;
	link_double_t* prev;
};

static void setting_set_name(setting_t* setting, const char* name)
{
	strncpy(setting->name, name, SETTING_MAX_NAME_LEN);
	setting->name[SETTING_MAX_NAME_LEN - 1] = 0;
}

static link_double_t* setting_list_head = NULL;

static void setting_list_add(setting_t* setting)
{
	link_double_t* link = ((link_double_t*)setting) - 1;
	assert(link->next == NULL);
	assert(link->prev == NULL);

	link->next = setting_list_head;
	if (link->next != NULL)
	{
		link->next->prev = link;
	}
	setting_list_head = link;
}

static link_double_t* setting_list_remove(setting_t* setting)
{
	link_double_t* link = ((link_double_t*)setting) - 1;

	if (link->next != NULL)
	{
		link->next->prev = link->prev;
	}

	if (link->prev != NULL)
	{
		link->prev->next = link->next;
	}
	else
	{
		setting_list_head = link->next;
	}

	return link;
}

//------------------------------------------------------------------------------
// Public API
//
setting_t* setting_create(const char* name)
{
	link_double_t* link = (link_double_t*)calloc(1, sizeof(link_double_t) + sizeof(setting_t));
	setting_t* setting = NULL;

	if (link != NULL)
	{
		link->next = NULL;
		link->prev = NULL;
		setting = (setting_t*)(link + 1);
		setting_set_name(setting, name);
		setting->type = SETTING_TYPE_UNKNOWN;
		setting_list_add(setting);
	}

	return setting;
}

void setting_destroy(setting_t* setting)
{
	link_double_t* link = setting_list_remove(setting);
	free(link);
}

setting_t* setting_find(const char* name)
{
	link_double_t* link = setting_list_head;

	while (link != NULL)
	{
		setting_t* setting = (setting_t*)(link + 1);
		if (strncmp(name, setting->name, SETTING_MAX_NAME_LEN) == 0)
		{
			return setting;
		}
		else
		{
			link = link->next;
		}
	}

	return NULL;
}

void setting_init_as_int(setting_t* setting, int value)
{
	assert(setting->type == SETTING_TYPE_UNKNOWN);
	setting->type = SETTING_TYPE_INT;
	setting->value.integer = value;
}

void setting_init_as_float(setting_t* setting, float value)
{
	assert(setting->type == SETTING_TYPE_UNKNOWN);
	setting->type = SETTING_TYPE_FLOAT;
	setting->value.real = value;
}

void setting_init_as_enum(setting_t* setting, int value, enum_type_info_t* enum_type)
{
	assert(setting->type == SETTING_TYPE_UNKNOWN);
	assert(value >= 0 && value < enum_type->max_index);
	setting->type = SETTING_TYPE_ENUM;
	setting->value.integer = value;
	setting->type_info.enum_type_info = enum_type;
}

void setting_set_value_int(setting_t* setting, int value)
{
	assert(setting->type != SETTING_TYPE_UNKNOWN);

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

		case SETTING_TYPE_UNKNOWN:
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
