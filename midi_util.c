/*
 * midi_util.c
 *
 *  Created on: 30 Dec 2013
 *      Author: ntuckett
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  MIDI_DEVICE  "/dev/midi1"

void send_bcl(int device, const char* bcl_file_path)
{
	unsigned char header[9];
	unsigned char footer = 0xf7;
	char* bcl_line = NULL;

	header[0] = 0xf0;
	header[1] = 0x00;
	header[2] = 0x20;
	header[3] = 0x32;
	header[4] = 0x00;
	header[5] = 0x15;
	header[6] = 0x20;

	FILE* bcl_file = fopen(bcl_file_path, "r");

	if (bcl_file == NULL)
	{
		perror("Could not open BCL file");
		return;
	}

	int index = 0;
	size_t line_len;

	while (getline(&bcl_line, &line_len, bcl_file) != -1)
	{
		header[7] = (index & 0xff00) >> 8;
		header[8] = index & 0xff;

		write(device, &header, sizeof(header));
		write(device, bcl_line, line_len);
		write(device, &footer, sizeof(footer));
		index++;
	}

	free(bcl_line);
	fclose(bcl_file);
}

int main(int argc, char** argv) {
   char* midi_device = MIDI_DEVICE;

   if (argc > 1 ) {
      midi_device = argv[1];
   }

   // first open the sequencer device.
   int seqfd = open(midi_device, O_RDWR);
   if (seqfd == -1) {
      printf("Error: cannot open %s\n", MIDI_DEVICE);
      exit(1);
   }

   if (argc > 2) {
      if (strcmp(argv[2], "bcl_send") == 0)
          send_bcl(seqfd, argv[3]);
   }

   close(seqfd);
   return 0;
}
