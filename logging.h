/*
 * logging.h
 *
 *  Created on: 5 Jan 2014
 *      Author: ntuckett
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <log4c.h>

extern int logging_initialise();

#if defined(LOGGING_ENABLED)

extern log4c_category_t* log_cat;

#define LOG_INFO(msg, ...) log4c_category_info(log_cat, msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...) log4c_category_warn(log_cat, msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) log4c_category_error(log_cat, msg, ##__VA_ARGS__)

#else

#define LOG_INFO(msg ...)
#define LOG_WARN(msg ...)
#define LOG_ERROR(msg ...)

#endif

#endif /* LOGGING_H_ */
