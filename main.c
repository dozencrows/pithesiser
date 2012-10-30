/*
 * main.c
 *
 *  Created on: 29 Oct 2012
 *      Author: ntuckett
 */

#include <unistd.h>
#include "alsa.h"

int main()
{
	alsa_initialise("hw:0");
	sleep(1);
	alsa_deinitialise();
	return 0;
}
