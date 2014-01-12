/*
 * error_handler.c
 *
 *  Created on: 26 Dec 2013
 *      Author: ntuckett
 */

#include "error_handler.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <libconfig.h>
#include "logging.h"

#define MAX_ERROR_MSG_LEN	1024
#define MAX_ERROR_STACK		16

static int error_stack[MAX_ERROR_STACK];
static const char* error_msg_stack[MAX_ERROR_STACK];
static int error_stack_head = 0;

void push_last_error()
{
	if (error_stack_head < MAX_ERROR_STACK)
	{
		error_msg_stack[error_stack_head] = NULL;
		error_stack[error_stack_head++] = errno;
	}
}

void push_custom_error(const char* message)
{
	if (error_stack_head < MAX_ERROR_STACK)
	{
		error_msg_stack[error_stack_head] = message;
		error_stack[error_stack_head++] = 0;
	}
}

void pop_error_report(const char* user_message)
{
	if (error_stack_head > 0)
	{
		char error_msg[MAX_ERROR_MSG_LEN];
		error_stack_head--;
		if (error_stack[error_stack_head] != 0)
		{
			strerror_r(error_stack[error_stack_head], error_msg, MAX_ERROR_MSG_LEN);
			LOG_ERROR("%s: %s", user_message, error_msg);
		}
		else
		{
			LOG_ERROR("%s: %s", user_message, error_msg_stack[error_stack_head]);
		}
	}
}

#define MAX_MESSAGE_BUFFER_LEN	4096
void setting_error_report(config_setting_t *setting, const char* message, ...)
{
	char message_buffer[MAX_MESSAGE_BUFFER_LEN];
	va_list arg_list;
	va_start(arg_list, message);
	vsnprintf(message_buffer, MAX_MESSAGE_BUFFER_LEN, message, arg_list);
	va_end(arg_list);
	message_buffer[MAX_MESSAGE_BUFFER_LEN - 1] = 0;
	LOG_ERROR("%s at line %d in %s", message_buffer, config_setting_source_line(setting), config_setting_source_file(setting));
}

