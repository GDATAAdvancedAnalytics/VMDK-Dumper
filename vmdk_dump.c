/* Copyright 2024 G DATA Advanced Analytics GmbH */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "vmdk.h"

int inflate_data_into_file(const uint8_t *compressed_data, size_t compressed_size, FILE *file, size_t *inflated_size);

static void print_header(const SparseExtentHeader *header)
{
    puts("=== VMware Hosted Sparse Extent ===");
    printf("Version: %d\n", header->version);
    printf("Flags: 0x%X\n", header->flags);
    puts("");
    printf("Capacity: 0x%lX\n", header->capacity);
    printf("Grain Size: 0x%lX\n", header->grainSize);
    printf("Descriptor Offset: 0x%lX\n", header->descriptorOffset);
    printf("Descriptor Size: 0x%lX\n", header->descriptorSize);
    printf("Number of entries per grain table: %d\n", header->numGTEsPerGT);
    printf("RGD Offset: 0x%lX\n", header->rgdOffset);
    printf("GD Offset: 0x%lX\n", header->gdOffset);
    printf("Overhead: 0x%lX\n", header->overHead);
    puts("");
    if (header->uncleanShutdown) {
        puts("Shutdown was unclean: Yes");
    } else {
        puts("Shutdown was unclean: No");
    }
    if (header->compressAlgorithm == COMPRESSION_NONE) {
        puts("Compression Algorithm: Not compressed");
    } else if (header->compressAlgorithm == COMPRESSION_DEFLATE) {
        puts("Compression Algorithm: Deflate");
    } else {
        printf("Compression Algorithm: %d (unknown!)\n", header->compressAlgorithm);
    }
    puts("");
}

static void fill_sparse(FILE *out_file, uint64 num_bytes)
{
    long pos = ftell(out_file);
    if (ftruncate(fileno(out_file), pos + num_bytes) != 0) {
        perror("Error filling sparse data using ftruncate64");
        exit(1);
    }
    if (fseek(out_file, 0, SEEK_END) != 0) {
        perror("fseek failed after extending file");
        exit(1);
    }
}

static int process_grain(FILE *file, int compressAlgorithm, uint64 *prev_lba, FILE *out_file)
{
    long pos = ftell(file);
    Marker marker;
    if (fread(&marker, sizeof(marker), 1, file) != 1) {
        perror("Unable to read marker. Premature end of file?");
        return 0;
    }

    if (marker.size != 0) {
        printf("%lX: LBA %lX, 0x%X bytes", pos, marker.val, marker.size);
        if (out_file) {
            if (*prev_lba != (uint64)-1 && *prev_lba < marker.val - 0x80) {
                fill_sparse(out_file, (marker.val - *prev_lba - 0x80) * SECTOR_SIZE);
            }
            *prev_lba = marker.val;

            uint8_t *file_data = malloc(marker.size);
            if (file_data == NULL) {
                fprintf(stderr, "\nAllocation error!\n");
                return 0;
            }
            if (fread(file_data, marker.size, 1, file) != 1) {
                perror("\nUnable to read grain data. Premature end of file?");
                return 0;
            }

            size_t out_size;
            if (compressAlgorithm == COMPRESSION_DEFLATE) {
                if (inflate_data_into_file(file_data, marker.size, out_file, &out_size) != 0) {
                    return 0; // decompression failed
                }
            } else {
                if (fwrite(file_data, marker.size, 1, out_file) != 1) {
                    perror("\nError copying uncompressed data");
                    return 0;
                }
            }

            printf(" -> 0x%lX bytes\n", out_size);

            free(file_data);

            long curPos = ftell(file);
            if (curPos & (SECTOR_SIZE - 1)) {
                fseek(file, (curPos + SECTOR_SIZE) & -SECTOR_SIZE, SEEK_SET);
            }
        } else {
            puts("");
            long newPos = ftell(file) + marker.size;
            if (newPos & (SECTOR_SIZE - 1)) {
                newPos = (newPos + SECTOR_SIZE) & -SECTOR_SIZE;
            }
            fseek(file, newPos, SEEK_SET);
        }
        return 1;
    } else {
        int type;
        if (fread(&type, 4, 1, file) != 1) {
            perror("Failed to read type int for marker");
            return 0;
        }

        if (type == MARKER_FOOTER) {
            puts("Footer marker encountered - stopping processing.");
            return 0;
        }
        printf("Skipping marker of type %d\n", type);
        // Skip the marker sector and all sectors occupied by the metadata (marker.val aka numSectors).
        if (fseek(file, SECTOR_SIZE - sizeof(marker) - 4 + marker.val * SECTOR_SIZE, SEEK_CUR) != 0) {
            perror("Failed to skip metadata marker");
            return 0;
        }
        return 1;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <vmdk filename> (<raw output filename>)\n", argv[0]);
        return 1;
    }

    SparseExtentHeader header;
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Unable to open the input file for reading.");
        return 1;
    }
    if (fread(&header, sizeof(header), 1, file) != 1) {
        perror("Unable to read the header.");
        return 1;
    }

    if (header.magicNumber != SPARSE_MAGICNUMBER) {
        fprintf(stderr, "Header magic mismatch, expected sparse magic, got 0x%08X.\n", header.magicNumber);
        return 1;
    }

    print_header(&header);

    if (header.overHead == 0) {
        fprintf(stderr, "Encountered implausible metadata size of 0!\n");
        return 1;
    }
    if (header.compressAlgorithm != COMPRESSION_NONE && header.compressAlgorithm != COMPRESSION_DEFLATE) {
        fprintf(stderr, "Encountered unsupported compression algorithm!\n");
        return 1;
    }

    // Skip the metadata sectors.
    if (fseek(file, header.overHead * SECTOR_SIZE, SEEK_SET) != 0) {
        perror("Skipping metadata failed");
        return 1;
    }

    FILE *dump = NULL;
    if (argc > 2) {
        dump = fopen(argv[2], "w");
        if (!dump) {
            perror("Unable to open the output file for writing.");
            return 1;
        }
    }

    uint64 lba = (uint64)-1;
    while (1) {
        if (!process_grain(file, header.compressAlgorithm, &lba, dump))
            break;
    }

    if (dump) fclose(dump);
    fclose(file);
}
