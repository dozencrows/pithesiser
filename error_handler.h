/*
 * error_handler.h
 *
 *  Created on: 26 Dec 2013
 *      Author: ntuckett
 */

#ifndef ERROR_HANDLER_H_
#define ERROR_HANDLER_H_

extern void push_last_error();
extern void push_custom_error(const char* message);
extern void pop_error_report(const char* message);

#endif /* ERROR_HANDLER_H_ */
