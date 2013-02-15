/*
 * code_timing_tests.c
 *
 *  Created on: 15 Feb 2013
 *      Author: ntuckett
 */
#include "code_timing_tests.h"

static const char* CFG_FILTER_TIMING_TEST_COUNT = "filter_timing_test_count";

void code_timing_tests_main(config_setting_t *config)
{
	int filter_timing_test_count;

	if (config_setting_lookup_int(config, CFG_FILTER_TIMING_TEST_COUNT, &filter_timing_test_count) == CONFIG_TRUE)
	{
		filter_timing_test(filter_timing_test_count);
	}
}
