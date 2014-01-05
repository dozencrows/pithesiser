/*
 * logging.c
 *
 *  Created on: 5 Jan 2014
 *      Author: ntuckett
 */
#include "logging.h"
#include <syslog.h>
#include <stdlib.h>

#include "system_constants.h"

#if defined(LOGGING_ENABLED)
static const char* SYSLOG_IDENT = "pithesiser";
log4c_category_t* log_cat = NULL;
#endif

void logging_deinitialise()
{
	log4c_fini();
}

int logging_initialise()
{
#if defined(LOGGING_ENABLED)
	openlog(SYSLOG_IDENT, LOG_NDELAY, LOG_USER);
	atexit(closelog);

	if (log4c_init())
	{
		printf("Log4c initialisation failed");
		return RESULT_ERROR;
	}
	else
	{
		atexit(logging_deinitialise);
		log_cat = log4c_category_get("pithesiser.log");
	}
#endif

	return RESULT_OK;
}
