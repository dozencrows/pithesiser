/*
 * main.c
 *
 *  Created on: 29 Oct 2012
 *      Author: ntuckett
 */

#include "alsa.h"

int main()
{
	alsa_initialise("hw:0");
	alsa_deinitialise();
	return 0;
}
