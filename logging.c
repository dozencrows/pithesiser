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
