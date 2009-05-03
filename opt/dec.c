#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <arpa/inet.h>
#include <bzlib.h>

#define debug(...) fprintf(stderr, __VA_ARGS__);

char *bunzip2(char *input, int input_len, int *output_len)
{
	bz_stream *stream = calloc(1, sizeof(bz_stream));

	switch (BZ2_bzDecompressInit(stream, 0, 0)) {
	case BZ_CONFIG_ERROR: err(1, "the library has been mis-compiled");
	case BZ_PARAM_ERROR:  err(1, "Parameter error");
	case BZ_MEM_ERROR:    err(1, "insufficient memory is available");
	//case BZ_OK:           debug("success\n"); break;
	//default:              debug("unknown\n"); break;
	}

	int   status;
	int   output_size = 512;
	char *output      = NULL;

	do {
		stream->next_in   = input       + stream->total_in_lo32;
		stream->avail_in  = input_len   - stream->total_in_lo32;
		output_size *= 2;
		output       = realloc(output, output_size);
		//debug("alloc %d\n", output_size);
		stream->next_out  = output      + stream->total_out_lo32;
		stream->avail_out = output_size - stream->total_out_lo32;
		//debug("decompressing..\n"
		//      "  next_in   = %p\n"
		//      "  avail_in  = %u\n"
		//      "  next_out  = %p\n"
		//      "  avail_out = %u\n",
		//      stream->next_in,
		//      stream->avail_in,
		//      stream->next_out,
		//      stream->avail_out);
	} while ((status = BZ2_bzDecompress(stream)) == BZ_OK && output_size < 1024*1024);

	// debug("done with status %d = %d\n", status, BZ_STREAM_END);

	*output_len = stream->total_out_lo32;
	BZ2_bzDecompressEnd(stream);
	return output;
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("usage: ./dec <input> <output>\n");
		return 0;
	}

	FILE *input  = fopen(argv[1], "r");
	FILE *output = fopen(argv[2], "w+");
	if (!input)  err(1, "error opening input");
	if (!output) err(1, "error opening output");

	int st;
	int size = 0;
	char *buf = malloc(24);

	/* Clear header */
	//debug("reading header\n");
	fread (buf, 1, 24, input);
	fwrite(buf, 1, 24, output);

	//debug("reading body\n");
	while ((st = fread(&size, 1, 4, input))) {
		//debug("read %u bytes\n", st);
		//fwrite(&size, 1, 4, output); // DEBUG
		size = abs(ntohl(size));
		if (size < 0)
			return 0;
		//debug("size = %x\n", size);
		if (size > 10*1024*1024)
			err(1, "sanity check failed, buf is to big: %d", size);
		buf = realloc(buf, size);
		fread (buf, 1, size, input);
		//fwrite(buf, 1, size, output); // DEBUG

		int dec_len;
		char *dec = bunzip2(buf, size, &dec_len);
		// debug("decompressed %u bytes\n", dec_len);
		fwrite(dec, 1, dec_len, output);
		free(dec);
	}

	return 0;
}
