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
 * master_time.c
 *
 *  Created on: 6 Dec 2012
 *      Author: ntuckett
 */
#include "master_time.h"
#include <time.h>

struct timespec timespec_diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

int32_t	get_elapsed_time_ms()
{
	static int base_set = 0;
	static struct timespec base_tspec;
	struct timespec tspec;

	if (base_set == 0)
	{
		base_set = 1;
		clock_gettime(CLOCK_REALTIME, &base_tspec);
	}

	clock_gettime(CLOCK_REALTIME, &tspec);
	struct timespec diff = timespec_diff(base_tspec, tspec);

	return (diff.tv_nsec / 1000000) + (diff.tv_sec * 1000);
}

int32_t	get_elapsed_cpu_time_ns()
{
	static int base_set = 0;
	static struct timespec base_tspec;
	struct timespec tspec;

	if (base_set == 0)
	{
		base_set = 1;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &base_tspec);
	}

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tspec);
	struct timespec diff = timespec_diff(base_tspec, tspec);

	return diff.tv_nsec + (diff.tv_sec * 1000000000);
}
