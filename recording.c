/*
 * recording.c
 *
 *  Created on: 2 Mar 2013
 *      Author: ntuckett
 */

#include "recording.h"
#include <sndfile.h>
#include "system_constants.h"
#include "gfx_event.h"
#include "gfx_event_types.h"

static const char* CFG_RECORDING = "recording";
static const char* CFG_OUTPUT_FILE = "output_file";

static SNDFILE *sndfile = NULL;
static gfx_object_t gfx_object;

//--------------------------------------------------------------------------------------------------------------
// Event handlers
//
static void wave_event_handler(gfx_event_t *event, gfx_object_t *receiver)
{
	sf_writef_short(sndfile, (short*)event->ptr, event->size / BYTES_PER_SAMPLE);
}

static void silence_event_handler(gfx_event_t *event, gfx_object_t *receiver)
{
	sf_seek(sndfile, event->size / BYTES_PER_SAMPLE, SEEK_CUR);
}

//--------------------------------------------------------------------------------------------------------------

static void configure(config_setting_t *setting_recording, int gfx_event_id)
{
	const char *output_file;

	if (config_setting_lookup_string(setting_recording, CFG_OUTPUT_FILE, &output_file) != CONFIG_TRUE)
	{
		printf("Recording not started: no output file specified\n");
	}

	SF_INFO sndinfo;

	sndinfo.samplerate = 44100;
	sndinfo.channels = 2;
	sndinfo.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
	sndfile = sf_open(output_file, SFM_WRITE, &sndinfo);

	if (sndfile != NULL)
	{
		gfx_object.id = gfx_event_id;
		gfx_register_event_receiver_handler(GFX_EVENT_WAVE, wave_event_handler, &gfx_object);
		gfx_register_event_receiver_handler(GFX_EVENT_SILENCE, silence_event_handler, &gfx_object);
	}
	else
	{
		printf("Recording not started: %s\n", sf_strerror(sndfile));
	}
}

void recording_initialise(config_t *config, int gfx_event_id)
{
	config_setting_t *setting_recording = config_lookup(config, CFG_RECORDING);
	if (setting_recording != NULL)
	{
		configure(setting_recording, gfx_event_id);
	}
}

void recording_deinitialise()
{
	if (sndfile != NULL)
	{
		sf_write_sync(sndfile);
		sf_close(sndfile);
		sndfile = NULL;
	}
}
