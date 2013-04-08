/*
 * code_timing_tests.c
 *
 *  Created on: 15 Feb 2013
 *      Author: ntuckett
 */
#include "code_timing_tests.h"
#include "tests/filter_timing_test.h"
#include "tests/waveform_timing_test.h"
#include "tests/output_conversion_timing_test.h"
#include "tests/mixer_timing_test.h"

static const char* CFG_FILTER_TIMING_TEST_COUNT = "filter_timing_test_count";
static const char* CFG_WAVEFORM_TIMING_TEST_COUNT = "waveform_timing_test_count";
static const char* CFG_OUTPUT_CONVERSION_TIMING_TEST_COUNT = "output_conversion_timing_test_count";
static const char* CFG_MIXER_TIMING_TEST_COUNT = "mixer_timing_test_count";

void code_timing_tests_main(config_setting_t *config)
{
	int test_count;

	if (config_setting_lookup_int(config, CFG_FILTER_TIMING_TEST_COUNT, &test_count) == CONFIG_TRUE)
	{
		filter_timing_test(test_count);
	}

	if (config_setting_lookup_int(config, CFG_WAVEFORM_TIMING_TEST_COUNT, &test_count) == CONFIG_TRUE)
	{
		waveform_timing_test(test_count);
	}

	if (config_setting_lookup_int(config, CFG_OUTPUT_CONVERSION_TIMING_TEST_COUNT, &test_count) == CONFIG_TRUE)
	{
		output_conversion_timing_test(test_count);
	}

	if (config_setting_lookup_int(config, CFG_MIXER_TIMING_TEST_COUNT, &test_count) == CONFIG_TRUE)
	{
		mixer_timing_test(test_count);
	}

}

