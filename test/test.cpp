#include <cstdint>
#include <cstdio>
#include <malloc.h>

#define SIMPLE_RESOURCE_COMPILER_IMPLEMENTATION
#include "simple_resource_compiler.h"

#define SRC_RESOURCE_TEST_IMPLEMENTATION
#include "test.src.h"

#define SRC_RESOURCE_FOO_IMPLEMENTATION
#include "foo.src.h"

constexpr const char* TEST_SRC = "test.src";

int main(int argc, char** argv) noexcept
{
	FILE* src = fopen(TEST_SRC, "rb");
	if (!src) return -1;

	src_main_header srcHeader = { 0 };
	fread(&srcHeader, sizeof(src_main_header), 1, src);
	if (!src_validate_header(&srcHeader)) {
		printf("ERROR: Header didn't validate.\n");
		return -1;
	}

	int nameBufSize = 1024;
	char* nameBuf = (char*)malloc(nameBufSize);

	for (int i = 0; i < srcHeader.subResourceCount; i += 1) {
		src_resource_header sub = { 0 };
		fread(&sub, sizeof(src_resource_header), 1, src);
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

	printf("\n\n\n=======================================================\n");
	printf("Test successful.\n\n\n");

	////////////////////////////////////////////////////////////
	src = fopen(TEST_SRC, "rb");
	if(!src)  {
		printf("Failed to open \"%s\"\n", TEST_SRC);
		return -1;
	}
	for(int32_t id=0; id < SRC_RESOURCE_TEST_ID::SRC_TEST_COUNT; id += 1) {
		src_resource_header header = {0};
		const char* nameFromTable = src_get_test_resource_name(id);
		size_t offsetIntoFile = src_get_test_resource_offset(id);
		fseek(src, offsetIntoFile, SEEK_SET);
		if(ferror(src)) {
			printf("Seek failed.\n");
			return -1;
		}
		fread(&header, sizeof(src_resource_header), 1, src);
		if (!src_validate_sub_header(&header)) {
			printf("Error: sub resource header failed to validate.\n");
			return -1;
		}	

		if (nameBufSize < header.nameLen) {
			free(nameBuf);
			nameBuf = (char*)malloc(header.nameLen);
		}
		fread(nameBuf, 1, header.nameLen, src);
		if (nameBuf[header.nameLen-1] != '\0') {
			printf("Error: Name not 0 terminated.\n");
			return -1;
		}

		if(strcmp(nameBuf, nameFromTable) != 0) {
			printf("Error: Name and name from table didn't match.\n");
			return -1;
		}
	}
	fclose(src);
	return 0;
}