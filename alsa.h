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
