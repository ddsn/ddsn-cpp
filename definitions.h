#ifndef DDSN_DEFINITIONS_H
#define DDSN_DEFINITIONS_H

typedef char               CHAR;
typedef unsigned char      BYTE;
typedef short              INT16;
typedef unsigned short     UINT16;
typedef int                INT32;
typedef unsigned int       UINT32;
typedef long long          INT64;
typedef unsigned long long UINT64;

#define DDSN_MESSAGE_TYPE_END    0
#define DDSN_MESSAGE_TYPE_STRING 1
#define DDSN_MESSAGE_TYPE_BYTES  2
#define DDSN_MESSAGE_TYPE_CLOSE  3
#define DDSN_MESSAGE_TYPE_ERROR	 4

#define DDSN_MESSAGE_CHUNK_MAX_SIZE    8 * 1024 * 1024
#define DDSN_MESSAGE_STRING_MAX_LENGTH 1024

#endif