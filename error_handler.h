/*
 * error_handler.h
 *
 *  Created on: 26 Dec 2013
 *      Author: ntuckett
 */

#ifndef ERROR_HANDLER_H_
#define ERROR_HANDLER_H_

typedef struct config_setting_t config_setting_t;

extern void push_last_error();
extern void push_custom_error(const char* message);
extern void pop_error_report(const char* message);
extern void setting_error_report(config_setting_t *setting, const char* message, ...);

#endif /* ERROR_HANDLER_H_ */
