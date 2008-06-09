/* Prototype stuff for parsing Level-II data */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <arpa/inet.h>

struct packet {
	short          junk1[6];
	unsigned short size;
	unsigned char  id, type;
	unsigned short seq, gen_date;
	unsigned int   gen_time;
	unsigned short num_seg, seg;
	unsigned int   coll_time;
	unsigned short coll_date, range, angle, radial, rad_status, elev_angle;
	unsigned short elev_num;
	short          first_refl, first_dopp;
	unsigned short refl_size, dopp_size;
	unsigned short num_refl_gate, num_dopp_gate, sector;
	float          gain;
	unsigned short refl_ptr, vel_ptr, spec_ptr, dopp_res, pattern;
	short          junk2[4];
	unsigned short refl_ptr_rda, vel_ptr_rda, spec_ptr_rda, nyquist, atten;
	short          thresh;
	short          junk3[17];
	unsigned char  data[2304];
	float          dbz[460];
};

typedef struct {
	/* 24 bytes */
	char version[4];
	char unknown0[16];
	char station[4];
} __attribute__ ((packed)) header_t;

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("usage: %s <level2-data>\n", argv[0]);
		return 0;
	}

	/* Read header */
	FILE *fd = fopen(argv[1], "r");
	if (fd == NULL)
		error(1, errno, "Error opening files `%s'", argv[1]);
	header_t header;
	if (1 != fread(&header, sizeof(header_t), 1, fd))
		error(1, errno, "Error reading header");

	/* Cut up the bzips */
	int file_num = 0;
	char filename[16];
	char *buf = NULL;
	FILE *outfile = NULL;
	unsigned int size = 0;

	while ((int)size >= 0) {
		if (1 != fread(&size, 4, 1, fd))
			break; //error(1, errno, "Error reading size, pos=%x", (unsigned int)ftell(fd));
		size = ntohl(size);

		/* Read data */
		;
		if (NULL == (buf = (char *)realloc(buf, size)))
			error(1, errno, "cannot allocate `%d' bytes for buffer", size);
		if (size != fread(buf, 1, size, fd))
			error(1, errno, "error reading from input file");

		/* Decmopress data */
		//char *block = (char *)malloc(8192), *oblock = (char *)malloc(262144);
		//error = BZ2_bzBuffToBuffDecompress(oblock, &olength, block, length, 0, 0);

		/* Write data */
		snprintf(filename, 16, "%d.bz2", file_num);
		outfile = fopen(filename, "w+");
		fwrite(buf, 1, size, outfile);
		fclose(outfile);

		//fprintf(stderr, "wrote `%d' bytes to file `%s'\n", size, filename);

		/* iterate */
		file_num++;
	}

	return 0;
}
