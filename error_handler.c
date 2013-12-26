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
			fprintf(stderr, "%s: %s", user_message, error_msg);
		}
		else
		{
			fprintf(stderr, "%s: %s", user_message, error_msg_stack[error_stack_head]);
		}
	}
}
