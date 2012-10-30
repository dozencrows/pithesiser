/*
 * alsa.c
 *
 *  Created on: 29 Oct 2012
 *      Author: ntuckett
 *
 *  Module to initialise and manage sound output via ALSA.
 *  Specifically written for the situation in hand:
 *
 *  Configuration:
 *  	2 channels.
 *  	16-bit little-endian samples.
 *  	44.1kHz sample rate.
 *
 *  Device name is selectable.
 *
 *  Proposed operation:
 *
 *  	Initialisation:
 *			period size is set to minimum.
 *			buffer size is set to 2 periods.
 *			two application side buffers are created, sized for one period, and set to silence.
 *			one application side "silence" buffer is created.
 *			next audible buffer index set to zero.
 *			app buffer 0 set to empty.
 *			app buffer 1 set to empty.
 *			silence buffer is written to PCM twice.
 *			async handler is set up.
 *			PCM is started.
 *
 *		Loop:
 *			Find next empty buffer
 *				write sample data to it and set to full.
 *			When no empty buffers left
 *				Sleep on empty buffer signal.
 *
 *		Handler:
 *			If next audible buffer is empty, write silence.
 *			Else
 *				Writes next audible app buffer to PCM.
 *				Set next audible buffer to empty.
 *				Increments next audible buffer index, modulo 2.
 *			Send empty buffer signal.
 */

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define SAMPLE_RATE		44100
#define CHANNEL_COUNT	2
#define SAMPLE_FORMAT	SND_PCM_FORMAT_S16_LE
#define SAMPLE_ACCESS	SND_PCM_ACCESS_RW_INTERLEAVED

#define AUDIO_BUFFER_COUNT	2
#define PERIOD_COUNT		AUDIO_BUFFER_COUNT

snd_pcm_t* 				playback_handle;
snd_pcm_uframes_t		period_size_frames;
unsigned int			period_count;
int						period_dir;
snd_pcm_uframes_t		buffer_size_frames;
int						sample_bit_count;
snd_async_handler_t*	async_handler;

int		next_audible_buffer = 0;
void*	audio_buffer[AUDIO_BUFFER_COUNT] = { NULL, NULL };

static void alsa_error(const char* message, int error_code)
{
	fprintf(stderr, message, snd_strerror(error_code));
}

static void create_audio_buffers()
{
	int frame_size = (sample_bit_count / 8) * CHANNEL_COUNT;
	int audio_buffer_size = frame_size * period_size_frames;

	for (int i = 0; i < AUDIO_BUFFER_COUNT; i++)
	{
		audio_buffer[i] = malloc(audio_buffer_size);
		snd_pcm_format_set_silence(SAMPLE_FORMAT, audio_buffer[i], period_size_frames * CHANNEL_COUNT);
	}

	next_audible_buffer = 0;
}

static void async_callback(snd_async_handler_t* ahandler)
{
    snd_pcm_t* handle = snd_async_handler_get_pcm(ahandler);
    snd_pcm_sframes_t avail;
    int err;

    printf("audio cb\n");

    avail = snd_pcm_avail_update(handle);
    if (avail >= period_size_frames) {
		err = snd_pcm_writei(handle, audio_buffer[next_audible_buffer], period_size_frames);
	    next_audible_buffer++;
		if (err < 0) {
				alsa_error("Write error: %s\n", err);
				exit(EXIT_FAILURE);
		}
		if (err != period_size_frames) {
				printf("Write error: written %i expected %li\n", err, period_size_frames);
				exit(EXIT_FAILURE);
		}
    }
}

int alsa_initialise(const char* device_name)
{
	int error;
	snd_pcm_hw_params_t* hw_params;
	snd_pcm_sw_params_t* sw_params;

    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_sw_params_alloca(&sw_params);

    if ((error = snd_pcm_open (&playback_handle, device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
    	alsa_error("alsa_initialise: could not open device (%s)\n", error);
    	return -1;
    }

	if ((error = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
		alsa_error("cannot initialize hardware parameter structure (%s)\n", error);
		return -1;
	}

	if ((error = snd_pcm_hw_params_set_access(playback_handle, hw_params, SAMPLE_ACCESS)) < 0) {
		alsa_error("cannot set access type (%s)\n", error);
		return -1;
	}

	if ((error = snd_pcm_hw_params_set_format (playback_handle, hw_params, SAMPLE_FORMAT)) < 0)
	{
		alsa_error("cannot set sample format (%s)\n", error);
		return -1;
	}

	if ((error = snd_pcm_hw_params_set_channels (playback_handle, hw_params, CHANNEL_COUNT)) < 0)
	{
		alsa_error("cannot set channel count (%s)\n", error);
		return -1;
	}

	unsigned int sample_rate = SAMPLE_RATE;
	int dir = 0;

	if ((error = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &sample_rate, &dir)) < 0)
	{
		alsa_error("cannot set sample rate (%s)\n", error);
		return -1;
	}

	dir = 0;
	if ((error = snd_pcm_hw_params_set_period_size_first(playback_handle, hw_params, &period_size_frames, &dir)) < 0)
	{
		alsa_error("cannot set & get min period size (%s)\n", error);
		return -1;
	}

	period_count = PERIOD_COUNT;
	buffer_size_frames = period_size_frames * PERIOD_COUNT;
	if ((error = snd_pcm_hw_params_set_buffer_size(playback_handle, hw_params, buffer_size_frames)) < 0)
	{
		alsa_error("cannot set buffer size periods (%s)\n", error);
		return -1;
	}

	if ((error = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
		alsa_error ("cannot set hw parameters (%s)\n", error);
		return -1;
	}

	if ((sample_bit_count = snd_pcm_hw_params_get_sbits (hw_params)) < 0) {
		alsa_error ("cannot get sample size parameters (%s)\n", sample_bit_count);
		return -1;
	}

	if ((error = snd_pcm_sw_params_current(playback_handle, sw_params)) < 0) {
		alsa_error("cannot initialize software parameter structure (%s)\n", error);
		return -1;
	}

    error = snd_pcm_sw_params_set_start_threshold(playback_handle, sw_params, buffer_size_frames);
    if (error < 0) {
    	alsa_error("Unable to set start threshold mode for playback: %s\n", error);
    	return -1;
    }

    error = snd_pcm_sw_params_set_avail_min(playback_handle, sw_params, period_size_frames);
    if (error < 0) {
    	alsa_error("Unable to set avail min for playback: %s\n", error);
    	return -1;
    }

	if ((error = snd_pcm_sw_params (playback_handle, sw_params)) < 0) {
		alsa_error ("cannot set sw parameters (%s)\n", error);
		return -1;
	}

	create_audio_buffers();

	if ((error = snd_pcm_prepare(playback_handle)) < 0)
	{
		alsa_error("pcm prepare failed (%s)\n", error);
		return -1;
	}

	if ((error = snd_async_add_pcm_handler(&async_handler, playback_handle, async_callback, NULL)) < 0)
	{
		alsa_error("async handler setup failed error (%s)\n", error);
		return -1;
	}

	for (int i = 0; i < 1; i++)
	{
		snd_pcm_sframes_t avail = snd_pcm_avail(playback_handle);
		if (avail < 0) {
			alsa_error("initial write error (%s)\n", avail);
			return -1;
		}
		if (avail >= period_size_frames) {
			if ((error = snd_pcm_writei(playback_handle, audio_buffer[i], period_size_frames)) < 0)
			{
				alsa_error("initial write error (%s)\n", error);
				return -1;
			}
		}
	}

//	error = snd_pcm_start(playback_handle);
//	if (error < 0)
//	{
//		alsa_error("Start error: %s\n", error);
//		return -1;
//	}

	return 0;
}

void alsa_deinitialise()
{
	snd_pcm_drain(playback_handle);
	snd_pcm_close(playback_handle);

	for (int i = 0; i < AUDIO_BUFFER_COUNT; i++)
	{
		if (audio_buffer[i] != NULL)
		{
			free(audio_buffer[i]);
			audio_buffer[i] = NULL;
		}
	}
}
