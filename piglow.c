/*
 * piglow.c
 *
 *  Created on: 23 Dec 2013
 *      Author: ntuckett
 */
#include <wiringPi.h>
#include <piGlow.h>
#include <memory.h>
#include <libconfig.h>
#include "voice.h"

#define PIGLOW_LED_COUNT	6
#define PIGLOW_LED_MAX		255

static char last_led_brights[PIGLOW_LED_COUNT];
static int piglow_enabled = 0;
static int piglow_brightness = PIGLOW_LED_MAX;

void piglow_initialise(config_setting_t* config)
{
	if (config != NULL)
	{
		config_setting_lookup_bool(config, "enabled", &piglow_enabled);

		if (piglow_enabled)
		{
			double brightness = 1.0;
			config_setting_lookup_float(config, "brightness", &brightness);
			piglow_brightness = PIGLOW_LED_MAX * brightness;

			piGlowSetup(0);
			memset(last_led_brights, 0, sizeof(last_led_brights));
		}
	}
}

void piglow_update(voice_t* voice, int voice_count)
{
	if (piglow_enabled)
	{
		char led_brights[PIGLOW_LED_COUNT];
		memset(led_brights, 0, sizeof(led_brights));

		for (int i = 0; i < voice_count; i++)
		{
			if (voice[i].current_state != NOTE_NOT_PLAYING)
			{
				int led = (voice[i].note % 12) / 2;
				int bright = (voice[i].oscillator.level * PIGLOW_LED_MAX) / LEVEL_MAX;
				bright = bright * piglow_brightness / PIGLOW_LED_MAX;
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
}

void piglow_deinitialise()
{
	if (piglow_enabled)
	{
		piGlowLeg(0,0);
		piGlowLeg(1,0);
		piGlowLeg(2,0);
	}
}
