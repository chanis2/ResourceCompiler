#include <cstdint>
#include <cstdio>
#include <malloc.h>

#include <filesystem>

#define SIMPLE_RESOURCE_COMPILER_IMPLEMENTATION
#include "simple_resource_compiler.h"

constexpr const char* TEST_SRC = "test.src";

int main(int argc, char** argv) noexcept
{
	FILE* src = fopen(TEST_SRC, "rb");
	if (!src) return -1;

	SimpleResourceHeader srcHeader = { 0 };
	fread(&srcHeader, sizeof(SimpleResourceHeader), 1, src);
	if (!src_validate_header(&srcHeader)) {
		printf("ERROR: Header didn't validate.\n");
		return -1;
	}

	int nameBufSize = 1024;
	char* nameBuf = (char*)malloc(nameBufSize);

	for (int i = 0; i < srcHeader.subResourceCount; i += 1) {
		SimpleResourceSubResourceHeader sub = { 0 };
		fread(&sub, sizeof(SimpleResourceSubResourceHeader), 1, src);
		if (!src_validate_sub_header(&sub)) {
			printf("Error: sub resource header failed to validate.\n");
			return -1;
		}
		if (nameBufSize < sub.nameLen) {
			free(nameBuf);
			nameBuf = (char*)malloc(sub.nameLen);
		}
		fread(nameBuf, 1, sub.nameLen, src);
		if (nameBuf[sub.nameLen-1] != '\0') {
			printf("Error: Name not 0 terminated.\n");
			return -1;
		}
		
		printf("Found resource: \"%s\"\n", nameBuf);

		// skip data
		fseek(src, sub.resourceSize, SEEK_CUR);
	}
	free(nameBuf);

	fclose(src);
	return 0;
}