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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "logging.h"
#include "midi.h"
#include "system_constants.h"

#define CHANNEL_COUNT		16
#define MIDI_NOTE_COUNT		128
#define SYSEX_SLEEP_DELAY	200		// Sleep time in us after sending sysex on one channel to help read thread keep up
#define SEND_SLEEP_DELAY	200

static const char* MIDI_SYSEX_WRITE_ERROR = "MIDI sysex write error";
static const char* MIDI_SEND_ERROR = "MIDI send error";
typedef struct
{
	int		channel;
	int		controller_count;
	char*	controller_data;
	char*	controller_flag;
} channel_data;

channel_data channels[CHANNEL_COUNT] =
{
	{  0, 127, NULL },
	{  1, 127, NULL },
	{  2, 127, NULL },
	{  3, 127, NULL },
	{  4, 127, NULL },
	{  5, 127, NULL },
	{  6, 127, NULL },
	{  7, 127, NULL },
	{  8, 127, NULL },
	{  9, 127, NULL },
	{ 10, 127, NULL },
	{ 11, 127, NULL },
	{ 12, 127, NULL },
	{ 13, 127, NULL },
	{ 14, 127, NULL },
	{ 15, 127, NULL },
};

fixed_t note_frequency[MIDI_NOTE_COUNT] =
{
	2143237, 2270680, 2405702, 2548752, 2700309, 2860878, 3030994, 3211227, 3402176, 3604480, 3818814, 4045892, 4286473, 4541360, 4811404, 5097505, 5400618, 5721755, 6061989, 6422453, 6804352, 7208960, 7637627, 8091784, 8572947, 9082720, 9622807, 10195009, 10801236, 11443511, 12123977, 12844906, 13608704, 14417920, 15275254, 16183568, 17145893, 18165441, 19245614, 20390018, 21602472, 22887021, 24247954, 25689813, 27217409, 28835840, 30550508, 32367136, 34291786, 36330882, 38491228, 40780036, 43204943, 45774043, 48495909, 51379626, 54434817, 57671680, 61101017, 64734272, 68583572, 72661764, 76982457, 81560072, 86409886, 91548086, 96991818, 102759252, 108869635, 115343360, 122202033, 129468544, 137167144, 145323527, 153964914, 163120144, 172819773, 183096171, 193983636, 205518503, 217739269, 230686720, 244404066, 258937088, 274334289, 290647054, 307929828, 326240288, 345639545, 366192342, 387967272, 411037006, 435478539, 461373440, 488808132, 517874176, 548668578, 581294109, 615859655, 652480576, 691279090, 732384684, 775934544, 822074013, 870957077, 922746880, 977616265, 1035748353, 1097337155, 1162588218, 1231719311, 1304961152, 1382558180, 1464769368, 1551869087, 1644148025, 1741914154, 1845493760, 1955232530, 2071496706, 2194674310, 2325176436, 2463438621, 2609922305, 2765116361, 2929538736, 3103738174, 3288296050
};

int			midi_handle[MAX_MIDI_DEVICES];
int			midi_handle_count = 0;
pthread_t	midi_thread_handle;

#define MIDI_EVENT_BUFFER_SIZE	128
#define MIDI_EVENT_BUFFER_MASK	(MIDI_EVENT_BUFFER_SIZE - 1)

pthread_mutex_t	midi_buffer_lock = PTHREAD_MUTEX_INITIALIZER;

midi_event_t midi_event_buffer[MIDI_EVENT_BUFFER_SIZE];
int midi_event_buffer_write_index = 0;
int midi_event_buffer_read_index = 0;

void midi_push_event(int handle, unsigned char type, size_t data_length, unsigned char *data)
{
	pthread_mutex_lock(&midi_buffer_lock);

	int next_write_index = (midi_event_buffer_write_index + 1) & MIDI_EVENT_BUFFER_MASK;
	if (next_write_index != midi_event_buffer_read_index)
	{
		size_t copy_size;

		if (data_length < MIDI_EVENT_DATA_SIZE)
		{
			copy_size = data_length;
		}
		else
		{
			copy_size = MIDI_EVENT_DATA_SIZE;
		}

		midi_event_buffer[midi_event_buffer_write_index].device_handle = handle;
		midi_event_buffer[midi_event_buffer_write_index].type = type;
		memcpy(midi_event_buffer[midi_event_buffer_write_index].data, data, copy_size);
		midi_event_buffer_write_index = next_write_index;
	}

	pthread_mutex_unlock(&midi_buffer_lock);
}

midi_event_t *midi_pop_event(midi_event_t *event)
{
	midi_event_t *return_event = NULL;

	pthread_mutex_lock(&midi_buffer_lock);

	if (midi_event_buffer_read_index != midi_event_buffer_write_index) {
		memcpy(event, &midi_event_buffer[midi_event_buffer_read_index], sizeof(midi_event_t));
		return_event = event;
		midi_event_buffer_read_index = (midi_event_buffer_read_index + 1) & MIDI_EVENT_BUFFER_MASK;
	}
	pthread_mutex_unlock(&midi_buffer_lock);

	return return_event;
}

int midi_get_event_count()
{
	int event_count;

	pthread_mutex_lock(&midi_buffer_lock);

	if (midi_event_buffer_read_index <= midi_event_buffer_write_index)
	{
		event_count = midi_event_buffer_write_index - midi_event_buffer_read_index;
	}
	else
	{
		event_count = MIDI_EVENT_BUFFER_SIZE - midi_event_buffer_read_index + midi_event_buffer_write_index;
	}

	pthread_mutex_unlock(&midi_buffer_lock);

	return event_count;
}

static void midi_read_packet(int handle)
{
	unsigned char control_byte;
	if (read(handle, &control_byte, 1) < 1)
	{
		return;
	}

	unsigned char message = control_byte & 0xf0;

	if (message == 0xb0)
	{
		unsigned char control_data[2];

		if (read(handle, &control_data, sizeof(control_data)) == sizeof(control_data))
		{
			int channel_index = control_byte & 0x0f;
			channel_data* channel = &channels[channel_index];

			int control_index = control_data[0];
			if (control_index < channel->controller_count)
			{
				channel->controller_data[control_index] = control_data[1];
				channel->controller_flag[control_index] = 1;
			}
		}
	}
	else if (message == 0x80 || message == 0x90)
	{
		unsigned char control_data[2];
		if (read(handle, &control_data, sizeof(control_data)) == sizeof(control_data))
		{
			midi_push_event(handle, control_byte, sizeof(control_data), control_data);
		}
	}
}

static void* midi_thread()
{
	pthread_setname_np(midi_thread_handle, "pithesiser-midi");

	while (1)
	{
		fd_set rfds;
		int nfds = -1;

        FD_ZERO(&rfds);

        for (int i = 0; i < midi_handle_count; i++)
        {
            FD_SET(midi_handle[i], &rfds);
            if (midi_handle[i] > nfds)
            {
            	nfds = midi_handle[i];
            }
        }
        nfds++;

        if (select(nfds, &rfds, NULL, NULL, NULL) == -1)
        {
        	break;
        }

        for (int i = 0; i < midi_handle_count; i++)
        {
            if (FD_ISSET(midi_handle[i], &rfds))
            {
            	midi_read_packet(midi_handle[i]);
            }
        }
	}

	return NULL;
}

int midi_initialise(int device_count, const char** device_name)
{
	for (int i = 0; i < CHANNEL_COUNT; i++)
	{
		channels[i].controller_data = (char*) calloc(channels[i].controller_count, 1);
		channels[i].controller_flag = (char*) calloc(channels[i].controller_count, 1);
	}

	for (int i = 0; i < device_count; i++)
	{
		midi_handle[midi_handle_count] = open(device_name[i], O_RDWR);
		if (midi_handle[midi_handle_count] == -1)
		{
			LOG_ERROR("Midi error: cannot open device %s", device_name[i]);
		}
		else
		{
			midi_handle_count++;
		}
	}

	if (midi_handle_count > 0)
	{
		pthread_create(&midi_thread_handle, NULL, midi_thread, NULL);
		return 0;
	}
	else
	{
		return -1;
	}
}

int midi_get_raw_controller_changed(int channel_index, int controller_index)
{
	int controller_changed = 0;

	if (channel_index >= 0 && channel_index < CHANNEL_COUNT)
	{
		channel_data* channel = &channels[channel_index];

		if (controller_index >= 0 && controller_index < channel->controller_count)
		{
			controller_changed = channel->controller_flag[controller_index];
			channel->controller_flag[controller_index] = 0;
		}
	}

	return controller_changed;
}

int midi_get_raw_controller_value(int channel_index, int controller_index)
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

const unsigned char SYSEX_START = 0xf0;
const unsigned char SYSEX_END = 0xf7;

void midi_send_sysex(const char *sysex_message, size_t message_length)
{
	char *buffer = alloca(message_length + 2);
	buffer[0] = SYSEX_START;
	memcpy(buffer + 1, sysex_message, message_length);
	buffer[message_length + 1] = SYSEX_END;
	for (int i = 0; i < midi_handle_count; i++)
	{
		if (write(midi_handle[i], buffer, message_length + 2) != message_length + 2)
		{
			perror(MIDI_SYSEX_WRITE_ERROR);
		}

		usleep(SYSEX_SLEEP_DELAY);
	}
}

void midi_send(int device_handle, unsigned char command, unsigned char channel, unsigned char data0, unsigned char data1)
{
	unsigned char buffer[3];

	buffer[0] = (command & 0xf0) | (channel & 0x0f);
	buffer[1] = data0;
	buffer[2] = data1;

	if (device_handle == MIDI_ALL_DEVICES)
	{
		for (int i = 0; i < midi_handle_count; i++)
		{
			if (write(midi_handle[i], buffer, sizeof(buffer)) != sizeof(buffer))
			{
				perror(MIDI_SEND_ERROR);
			}

			usleep(SYSEX_SLEEP_DELAY);
		}
	}
	else
	{
		if (write(device_handle, buffer, sizeof(buffer)) != sizeof(buffer))
		{
			perror(MIDI_SEND_ERROR);
		}
		else
		{
			usleep(SEND_SLEEP_DELAY);
		}
	}
}

void midi_deinitialise()
{
	pthread_cancel(midi_thread_handle);
	pthread_join(midi_thread_handle, NULL);

	for (int i = 0; i < midi_handle_count; i++)
	{
		close(midi_handle[i]);
	}
	midi_handle_count = 0;

	for (int i = 0; i < CHANNEL_COUNT; i++)
	{
		free(channels[i].controller_data);
		channels[i].controller_data = NULL;
	}
}

fixed_t midi_get_note_frequency(int midi_note)
{
	if (midi_note < 0)
	{
		midi_note = 0;
	}
	else if (midi_note >= MIDI_NOTE_COUNT)
	{
		midi_note = MIDI_NOTE_COUNT -1;
	}
	return note_frequency[midi_note];
}

fixed_t midi_get_note_wavelength_samples(int midi_note)
{
	fixed_t note_frequency = midi_get_note_frequency(midi_note);
	fixed_wide_t sample_rate = (int64_t)SYSTEM_SAMPLE_RATE << (FIXED_PRECISION * 2);
	return (fixed_t)(sample_rate / note_frequency);
}
