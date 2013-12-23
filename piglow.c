/*
 * piglow.c
 *
 *  Created on: 23 Dec 2013
 *      Author: ntuckett
 */
#include <wiringPi.h>
#include <piGlow.h>
#include <memory.h>
#include "voice.h"

#define PIGLOW_LED_COUNT	6

static char last_led_brights[PIGLOW_LED_COUNT];

void piglow_initialise()
{
	piGlowSetup(0);
	memset(last_led_brights, 0, sizeof(last_led_brights));
}

void piglow_update(voice_t* voice, int voice_count)
{
	char led_brights[PIGLOW_LED_COUNT];
	memset(led_brights, 0, sizeof(led_brights));

	for (int i = 0; i < voice_count; i++)
	{
		if (voice[i].current_state != NOTE_NOT_PLAYING)
		{
			int led = (voice[i].note % 12) / 2;
			int bright = (voice[i].level_envelope_instance.last_level * 255) / LEVEL_MAX;
			if (bright > led_brights[led])
			{
				led_brights[led] = bright;
			}
		}
	}

	for (int i = 0; i < PIGLOW_LED_COUNT; i++)
	{
		if (led_brights[i] != last_led_brights[i])
		{
			piGlowRing(i, led_brights[i]);
		}
	}

	memcpy(last_led_brights, led_brights, sizeof(last_led_brights));
}

