#ifndef SIMPLE_RESOURCE_COMPILER
#define SIMPLE_RESOURCE_COMPILER
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#define SRC_RESOURCE_HEADER_VALUE "SRCDATA"
#define SRC_RESOURCE_VERSION 1

typedef struct {
	char header[8]; // == SRC_RESOURCE_HEADER_VALUE
	int32_t version; // == SRC_RESOURCE_VERSION
	size_t subResourceCount;
	// after the header follows
	/* first subresouce */
} SimpleResourceHeader;

int src_validate_header(SimpleResourceHeader* h);

#define SRC_SUB_RESOURCE_HEADER_VALUE "SUBDATA"

typedef struct {
	char header[8]; // == SRC_SUB_RESOURCE_HEADER_VALUE
	uint32_t id;
	size_t resourceSize;
	uint16_t nameLen;
	uint8_t flags;
	// after the header follows
	/* name */
	/* resourceData */
} SimpleResourceSubResourceHeader;

int src_validate_sub_header(SimpleResourceSubResourceHeader* h);

// djb2 http://www.cse.yorku.ca/~oz/hash.html
// this algorithm(k = 33) was first reported by dan bernstein many years ago in comp.lang.c.
// another version of this algorithm(now favored by bernstein) usesxor : hash(i) = hash(i - 1) * 33 ^ str[i];
// the magic of number 33 (why it works better than many other constants, prime or not) has never been adequately explained.
uint32_t djb2_hash(unsigned char* str);

#ifdef SIMPLE_RESOURCE_COMPILER_IMPLEMENTATION

#include <string.h>

int src_validate_header(SimpleResourceHeader* h)
{
	return h->version == SRC_RESOURCE_VERSION 
		&& strcmp(h->header, SRC_RESOURCE_HEADER_VALUE) == 0;
}

inline int src_validate_sub_header(SimpleResourceSubResourceHeader* h)
{
	return strcmp(h->header, SRC_SUB_RESOURCE_HEADER_VALUE) == 0;
}


uint32_t djb2_hash(unsigned char* str)
{
	uint32_t hash = 5381;
	uint32_t c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

#endif//SIMPLE_RESOURCE_COMPILER_IMPLEMENTATION

#ifdef __cplusplus
}
#endif
#endif//SIMPLE_RESOURCE_COMPILER