/*
 * Copyright (C) 2009-2010 Andy Spencer <andy753421@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <bzlib.h>

char *bunzip2(char *input, int input_len, int *output_len)
{
	bz_stream *stream = g_new0(bz_stream, 1);

	switch (BZ2_bzDecompressInit(stream, 0, 0)) {
	case BZ_CONFIG_ERROR: g_error("the library has been mis-compiled");
	case BZ_PARAM_ERROR:  g_error("Parameter error");
	case BZ_MEM_ERROR:    g_error("insufficient memory is available");
	//case BZ_OK:           g_debug("success\n"); break;
	//default:              g_debug("unknown\n"); break;
	}

	int   status;
	int   output_size = 512;
	char *output      = NULL;

	do {
		stream->next_in   = input       + stream->total_in_lo32;
		stream->avail_in  = input_len   - stream->total_in_lo32;
		output_size *= 2;
		output       = g_realloc(output, output_size);
		//g_debug("alloc %d\n", output_size);
		stream->next_out  = output      + stream->total_out_lo32;
		stream->avail_out = output_size - stream->total_out_lo32;
		//g_debug("decompressing..\n"
		//      "  next_in   = %p\n"
		//      "  avail_in  = %u\n"
		//      "  next_out  = %p\n"
		//      "  avail_out = %u\n",
		//      stream->next_in,
		//      stream->avail_in,
		//      stream->next_out,
		//      stream->avail_out);
	} while ((status = BZ2_bzDecompress(stream)) == BZ_OK && output_size < 1024*1024);

	// g_debug("done with status %d = %d\n", status, BZ_STREAM_END);

	*output_len = stream->total_out_lo32;
	BZ2_bzDecompressEnd(stream);
	return output;
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		g_print("usage: %s <input> <output>\n", argv[0]);
		return 0;
	}

	FILE *input  = fopen(argv[1], "rb");
	FILE *output = fopen(argv[2], "wb+");
	if (!input)  g_error("error opening input");
	if (!output) g_error("error opening output");

	int st;
	int size = 0;
	char *buf = g_malloc(24);

	/* Clear header */
	//g_debug("reading header\n");
	if (!fread (buf, 24, 1, input))
		g_error("error reading header");
	if (!fwrite(buf, 24, 1, output))
		g_error("error writing header");

	//g_debug("reading body\n");
	while ((st = fread(&size, 1, 4, input))) {
		//g_debug("size=%08x\n", size);
		//g_debug("read %u bytes\n", st);
		//fwrite(&size, 1, 4, output); // DEBUG
		size = ABS(g_ntohl(size));
		if (size < 0)
			return 0;
		//g_debug("size = %x\n", size);
		if (size > 20*1024*1024)
			g_error("sanity check failed, buf is to big: %d", size);
		buf = g_realloc(buf, size);
		if (!fread(buf, size, 1, input))
			g_error("error reading data");
		//fwrite(buf, 1, size, output); // DEBUG

		int dec_len;
		char *dec = bunzip2(buf, size, &dec_len);
		// g_debug("decompressed %u bytes\n", dec_len);
		if (!fwrite(dec, 1, dec_len, output))
			g_error("error writing data");
		g_free(dec);
	}

	return 0;
}
