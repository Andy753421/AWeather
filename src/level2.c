/* Prototype stuff for parsing Level-II data */
/*
 * TODO: ARGG, the packet sizses are all wrong..
 *       Check sizes of decompressed bzip files
 *       Split things back up to seperate files
 * The second bzip contains different size packets?
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <glib.h>
#include <bzlib.h>
#include "level2.h"

level2_packet_t **level2_split_packets(char *data, int *num_packets, int data_size)
{
	int packet_i = 0;
	level2_packet_t  *packet = (level2_packet_t *)data;
	int max_packets = 128;
	level2_packet_t **packets = malloc(sizeof(level2_packet_t *)*max_packets);
	while ((void *)packet < (void *)(data+data_size)) {
		/* Increase packets size if necessasairy */
		if (packet_i >= max_packets) {
			max_packets *= 2;
			packets = realloc(packets, sizeof(level2_packet_t *)*max_packets);
		}

		/* Fix byte order for packet */
		packet->size = g_ntohs(packet->size);
		packet->seq  = g_ntohs(packet->seq);
		// TODO: Convert the rest of the bytes

		/* Save packet location */
		packets[packet_i] = packet;

		/* Increment packet and packet_i */
		// new =                      old              + CTM + 2*size         + fcs
		//packet = (level2_packet_t *)( ((char *)packet) + 12  + 2*packet->size + 4 );
		packet++;
		packet_i++;
	}
	packets = realloc(packets, sizeof(level2_packet_t *)*packet_i);
	*num_packets = packet_i;
	return packets;
}

level2_packet_t *level2_decompress(char *raw_data, int *num_packets)
{
	/* Read header */
	FILE *fd = fopen(raw_data, "r");
	if (fd == NULL)
		error(1, errno, "Error opening files `%s'", raw_data);
	level2_header_t header;
	if (1 != fread(&header, sizeof(level2_header_t), 1, fd))
		error(1, errno, "Error reading header");

	/* Decompress the bzips
	 *   store the entire sequence starting at data
	 *   cur_data is for each individual bzip and is the last chunk of data */
	char *data = NULL;
	int data_size = 0; // size of previously decmpressed data
	char *bz2 = NULL;  // temp buf for bzipped data
	unsigned int _bz2_size = 0;
	while ((int)_bz2_size >= 0) {
		if (1 != fread(&_bz2_size, 4, 1, fd))
			break; //error(1, errno, "Error reading _bz2_size, pos=%x", (unsigned int)ftell(fd));
		_bz2_size = g_ntohl(_bz2_size);
		int bz2_size = abs(_bz2_size);

		/* Read data */
		if (NULL == (bz2 = (char *)realloc(bz2, bz2_size)))
			error(1, errno, "cannot allocate `%d' bytes for buffer", bz2_size);
		if (bz2_size != fread(bz2, 1, bz2_size, fd))
			error(1, errno, "error reading from input file");

		/* Decompress on individual bzip to the end of the sequence */
		unsigned int cur_data_size = 1<<17;
		int status = BZ_OUTBUFF_FULL;
		while (status == BZ_OUTBUFF_FULL) {
			cur_data_size *= 2;
			data = realloc(data, data_size + cur_data_size);
			status = BZ2_bzBuffToBuffDecompress(data + data_size, &cur_data_size, bz2, bz2_size, 0, 0);
		}
		if (status != BZ_OK)
			error(1, 1, "Error decompressing data");
		data_size += cur_data_size; // Add current chunk to decompressed data

		/* Debug */
		printf("data_size = %d, cur_data_size = %d\n", data_size, cur_data_size);
	}
	data = realloc(data, data_size); // free unused space at the end

	return (level2_packet_t *)data;
	//return level2_split_packets(data, num_packets, data_size);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("usage: %s <level2-data>\n", argv[0]);
		return 0;
	}

	int num_packets;
	level2_packet_t *packets = level2_decompress(argv[1], &num_packets);
	printf("read %d packets\n", num_packets);
	
	//FILE *output = fopen("output.dat", "w+");
	//fwrite(packets[i], 1, 18470816, output);
	//fclose(output);

	FILE *output = fopen("output.dat", "w+");
	int i;
	for (i = 0; i < 10000; i++) {
		printf("packet: size=%x, seq=%d\n", g_ntohs(packets[i].size), g_ntohs(packets[i].seq));
		fwrite("################", 1, 16, output);
		fwrite(packets+i, 1, sizeof(level2_packet_t), output);
	}
	fclose(output);

	return 0;
}
