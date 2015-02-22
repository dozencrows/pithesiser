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

