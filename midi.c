/*
 * midi.c
 *
 *  Created on: 30 Oct 2012
 *      Author: ntuckett
 *
 *
 *  Captures continuous controller changes in its own thread
 *
 *  0xBy CNumber CValue
 *
 *  where y is channel number (0-15)
 *  CNumber is controller id
 *  CValue is value (0-127)
 *
 *  Channel  0: max number of controllers is 127
 *  Channels 1-15: max number of controllers per channel is 17
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define CHANNEL_COUNT	16

typedef struct
{
	int		channel;
	int		controller_count;
	char*	controller_data;
} channel_data;

channel_data channels[CHANNEL_COUNT] =
{
	{  0, 127, NULL },
	{  1,  17, NULL },
	{  2,  17, NULL },
	{  3,  17, NULL },
	{  4,  17, NULL },
	{  5,  17, NULL },
	{  6,  17, NULL },
	{  7,  17, NULL },
	{  8,  17, NULL },
	{  9,  17, NULL },
	{ 10,  17, NULL },
	{ 11,  17, NULL },
	{ 12,  17, NULL },
	{ 13,  17, NULL },
	{ 14,  17, NULL },
	{ 15,  17, NULL },
};

int 		midi_handle;
pthread_t	midi_thread_handle;

static void* midi_thread()
{
	while (1) {
		unsigned char control_byte;

		if (read(midi_handle, &control_byte, 1) < 1)
		{
			break;
		}

		if ((control_byte & 0xf0) == 0xb0)
		{
			unsigned char control_data[2];

			if (read(midi_handle, &control_data, sizeof(control_data)) < sizeof(control_data))
			{
				break;
			}

			int channel_index = control_byte & 0x0f;
			channel_data* channel = &channels[channel_index];

			if (control_data[0] < channel->controller_count) {
				channel->controller_data[control_data[0]] = control_data[1];
			}
		}
	}

	return NULL;
}

int midi_initialise(const char* device_name)
{
	for (int i = 0; i < CHANNEL_COUNT; i++)
	{
		channels[i].controller_data = (char*) calloc(channels[i].controller_count, 1);
	}

	midi_handle = open(device_name, O_RDONLY);
	if (midi_handle == -1)
	{
		printf("Midi error: cannot open device %s\n", device_name);
		return -1;
	}

	pthread_create(&midi_thread_handle, NULL, midi_thread, NULL);

	return 0;
}

int midi_get_controller_value(int channel_index, int controller_index)
{
	int controller_value = -1;

	if (channel_index >= 0 && channel_index < CHANNEL_COUNT)
	{
		channel_data* channel = &channels[channel_index];

		if (controller_index >= 0 && controller_index < channel->controller_count)
		{
			controller_value = channel->controller_data[controller_index];
		}
	}

	return controller_value;
}

void midi_deinitialise()
{
	pthread_cancel(midi_thread_handle);
	pthread_join(midi_thread_handle, NULL);
	close(midi_handle);

	for (int i = 0; i < CHANNEL_COUNT; i++)
	{
		free(channels[i].controller_data);
		channels[i].controller_data = NULL;
	}
}
