/* Copyright 2024 G DATA Advanced Analytics GmbH */

#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint64 SectorType;
typedef uint8 Bool;
typedef struct SparseExtentHeader {
    uint32 magicNumber;
    uint32 version;
    uint32 flags;
    SectorType capacity;
    SectorType grainSize;
    SectorType descriptorOffset;
    SectorType descriptorSize;
    uint32 numGTEsPerGT;
    SectorType rgdOffset;
    SectorType gdOffset;
    SectorType overHead;
    Bool uncleanShutdown;
    char singleEndLineChar;
    char nonEndLineChar;
    char doubleEndLineChar1;
    char doubleEndLineChar2;
    uint16 compressAlgorithm;
    uint8 pad[433];
} __attribute__((__packed__)) SparseExtentHeader;

#define SPARSE_MAGICNUMBER 0x564d444b /* 'V' 'M' 'D' 'K' */
#define COMPRESSION_NONE 0
#define COMPRESSION_DEFLATE 1

#define SECTOR_SIZE 512  /* can this ever differ? */

typedef struct Marker {
    SectorType val; /* if size != 0: LBA */
    uint32 size;
    /* union {
        uint32 type;
        uint8 data[0];
    } u; */
} __attribute__((__packed__)) Marker;

// For type member:
#define MARKER_GT 1
#define MARKER_GD 2
#define MARKER_FOOTER 3
