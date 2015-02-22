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
 * Testing multi sound playback:
 * * Create abstraction for "sound data"
 * * Create abstraction for "sound player"
 * * Operations:
 *   * Create sound player
 *   * Create sound data & load from WAV
 *   * Assign sound data to sound player (sets up player format info)
 *   * Start sound player playing
 *   * Update sound player
 * * Get some suitable sounds (loops)
 * * Monitor performance
 * 
 * 
 * * Requires:
 *   * Multiple PCM playback handles open at once
 *   * Cmd line arg handling to load up multiple sound data files
 *   * Set up of player per sound data file
 *   * Playback & servicing of players
 *   * Use of ALSA in interrupt driven mode
 *   * Make sound playback support looping
 *   * Sounds like we want to move to C++, introduce some separate code modules, and start some test-driven development? Build that into just what's needed to 
 *     make this performance investigation work (rather than make it fully featured).
 */

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#include <ctype.h>
#include <stdbool.h>
#include <endian.h>
#include <byteswap.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#elif __BYTE_ORDER == __BIG_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#else
#error "Wrong endian"
#endif

#define WAV_RIFF		COMPOSE_ID('R','I','F','F')
#define WAV_WAVE		COMPOSE_ID('W','A','V','E')
#define WAV_FMT			COMPOSE_ID('f','m','t',' ')
#define WAV_DATA		COMPOSE_ID('d','a','t','a')
#define WAV_PCM_CODE		1

typedef struct {
	u_int magic;		/* 'RIFF' */
	u_int length;		/* filelen */
	u_int type;		/* 'WAVE' */
} WaveHeader;

typedef struct {
	u_short format;		/* should be 1 for PCM-code */
	u_short modus;		/* 1 Mono, 2 Stereo */
	u_int sample_fq;	/* frequence of sample */
	u_int byte_p_sec;
	u_short byte_p_spl;	/* samplesize; 1 or 2 bytes */
	u_short bit_p_spl;	/* 8, 12 or 16 bit */
} WaveFmtBody;

typedef struct {
	u_int type;		/* 'data' */
	u_int length;		/* samplecount */
} WaveChunkHeader;

typedef struct {
	WaveHeader		header;
	WaveChunkHeader	fmt_header;
	WaveFmtBody		fmt_body;
	WaveChunkHeader	data_header;
	u_char sample_data[0];
} WavFile;

bool read_wav(const char* p_filename, WavFile** pp_wav)
{
	size_t file_size;
	void* p_file_data = NULL;
	WavFile* p_wav;
	bool result = false;
	
	// Open the file and attempt to read in header - any failure means error
	FILE* p_file = fopen(p_filename, "rb");
	
	if (p_file == NULL)
	{
		goto exit_error;
	}
	
	fseek(p_file, 0, SEEK_END);
	file_size = ftell(p_file);
	rewind(p_file);
	p_file_data = malloc(file_size);
	if (p_file_data == NULL)
	{
		goto exit_error;
	}
	
	if (fread(p_file_data, file_size, 1, p_file) != 1)
	{
		goto exit_error;
	}
	
	p_wav = (WavFile*)p_file_data;
	if (p_wav->header.magic != WAV_RIFF || p_wav->header.type != WAV_WAVE)
	{
		goto exit_error;
	}
	
	if (p_wav->fmt_header.type != WAV_FMT || p_wav->fmt_header.length != sizeof(WaveFmtBody))
	{
		goto exit_error;
	}

	if (p_wav->data_header.type != WAV_DATA)
	{
		goto exit_error;
	}

	*pp_wav = p_wav;
	result = true;

exit_error:
	if (!result)
	{
		*pp_wav = NULL;
		if (p_file_data != NULL)
		{
			free(p_file_data);
		}
	}
	
	if (p_file != NULL)
	{
		fclose(p_file);
	}
	return result;
}

bool setup_wav_hwparams(WavFile* p_wav, snd_pcm_t *playback_handle, snd_pcm_hw_params_t* p_hw_params)
{
	unsigned int rate=44100;
	int dir=0;
	snd_pcm_format_t format;
	int err;
	
	if ((err = snd_pcm_hw_params_set_channels (playback_handle, p_hw_params, p_wav->fmt_body.modus)) < 0) 
	{
		fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
		return false;
	}
	
	switch (p_wav->fmt_body.bit_p_spl) 
	{
		case 8:
		{
			format = SND_PCM_FORMAT_U8;
			break;
		}
		
		case 16:
		{
			format = SND_PCM_FORMAT_S16_LE;
			break;
		}
		case 24:
		{
			switch (p_wav->fmt_body.byte_p_spl / p_wav->fmt_body.modus) 
			{
				case 3:
				{
					format = SND_PCM_FORMAT_S24_3LE;
					break;
				}
				case 4:
				{
					format = SND_PCM_FORMAT_S24_LE;
					break;
				}
				default:
				{
					fprintf(stderr, " can't play WAVE-files with sample %d bits in %d bytes wide (%d channels)", p_wav->fmt_body.bit_p_spl, p_wav->fmt_body.byte_p_spl, p_wav->fmt_body.modus);
					return false;
				}
			}
			break;
		}
		case 32:
		{
			format = SND_PCM_FORMAT_S32_LE;
			break;
		}
		default:
		{
			fprintf(stderr, " can't play WAVE-files with sample %d bits wide", p_wav->fmt_body.bit_p_spl);
			return false;
		}
	}
	
	if ((err = snd_pcm_hw_params_set_format (playback_handle, p_hw_params, format)) < 0) 
	{
		fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
		return false;
	}

	rate = p_wav->fmt_body.sample_fq;
	
	if ((err = snd_pcm_hw_params_set_rate_near(playback_handle, p_hw_params, &rate, &dir)) < 0) 
	{
		fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
		return false;
	}

	return true;
}

int alsatest_main (int argc, char *argv[])
{
	int err;
	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params;
	WavFile* p_wavdata;

	if (!read_wav(argv[2], &p_wavdata))
	{
		fprintf (stderr, "error reading WAV %s \n", argv[2]); 
		exit (1);
	}

	if ((err = snd_pcm_open (&playback_handle, argv[1], SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n", 
			 argv[1],
			 snd_strerror (err));
		exit (1);
	}
	   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
			 
	if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	if (!setup_wav_hwparams(p_wavdata, playback_handle, hw_params))
	{
		exit (1);
	}
	
	if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	snd_pcm_hw_params_free (hw_params);

	if ((err = snd_pcm_prepare (playback_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	int frame_bytes = p_wavdata->fmt_body.byte_p_spl;
	snd_pcm_uframes_t num_frames_left = p_wavdata->data_header.length / frame_bytes;
	u_char* p_data = p_wavdata->sample_data;

	while (num_frames_left > 0)
	{
		snd_pcm_uframes_t num_frames_written = snd_pcm_writei(playback_handle, p_data, num_frames_left);
		
		if (num_frames_written < 0)
		{
			fprintf (stderr, "write to audio interface failed (%s)\n", snd_strerror (num_frames_written));
			exit (1);
		}
		else
		{
			num_frames_left -= num_frames_written;
			p_data += num_frames_written * frame_bytes;
		}
	}
	
	snd_pcm_drain (playback_handle);
	snd_pcm_close (playback_handle);
	free(p_wavdata);
	return 0;
}

