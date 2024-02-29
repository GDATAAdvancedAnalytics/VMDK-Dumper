/* Copyright 2024 G DATA Advanced Analytics GmbH */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>

#define CHUNK_SIZE 16384

int inflate_data_into_file(const uint8_t *compressed_data, size_t compressed_size, FILE *file, size_t *inflated_size)
{
    int ret;
    uint8_t out[CHUNK_SIZE];
    z_stream strm;

    *inflated_size = 0;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = compressed_size;
    strm.next_in = (uint8_t *)compressed_data;
    strm.avail_out = CHUNK_SIZE;
    strm.next_out = out;

    ret = inflateInit(&strm);
    if (ret != Z_OK) {
        fprintf(stderr, "\ninflateInit failed: %s\n", zError(ret));
        return ret;
    }

    do {
        ret = inflate(&strm, Z_NO_FLUSH);
        switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                fprintf(stderr, "\ninflate failed: %s\n", zError(ret));
                inflateEnd(&strm);
                return ret;
        }
        // Calculate the amount of data inflated.
        size_t inflated_chunk_size = CHUNK_SIZE - strm.avail_out;
        *inflated_size += inflated_chunk_size;
        // Write to file.
        if (fwrite(out, inflated_chunk_size, 1, file) != 1) {
            perror("\nFailed to write decompressed data");
            return Z_MEM_ERROR;
        }
        // Reset stream output buffer.
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = out;
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);

    return Z_OK;
}