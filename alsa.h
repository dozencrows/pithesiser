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
 * alsa.h
 *
 *  Created on: 29 Oct 2012
 *      Author: ntuckett
 */

#ifndef ALSA_H_
#define ALSA_H_

#define SAMPLE_RATE		44100
#define CHANNEL_COUNT	2
#define SAMPLE_FORMAT	SND_PCM_FORMAT_S16_LE
#define SAMPLE_ACCESS	SND_PCM_ACCESS_RW_INTERLEAVED

#define PERIOD_SIZE_MIN	-1

extern int alsa_initialise(const char* device_name, int period_size);
extern void alsa_deinitialise();

extern int alsa_get_samples_output();
extern int alsa_get_xruns_count();
extern void alsa_get_buffer_params(int buffer_index, void** data, int* sample_count);
extern void alsa_sync_with_audio_output();
extern int alsa_lock_next_write_buffer();
extern void alsa_unlock_buffer(int buffer_index);

#endif /* ALSA_H_ */
