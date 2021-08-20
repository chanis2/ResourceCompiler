#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define CUTE_FILES_IMPLEMENTATION
#include "cute_files.h"

#define SIMPLE_RESOURCE_COMPILER_IMPLEMENTATION
#include "simple_resource_compiler.h"

/// 
/// Usage: src.exe -t "resources/" -o "data.src"
/// 

#define LOGR_MSG(msg) PrintHelper(msg)
#define LOGF_MSG(fmt, ...) PrintHelper(fmt, __VA_ARGS__)

static void PrintHelper(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	puts("");
}

static void PrintUsage() {
	LOGR_MSG("Usage:\tsrc.exe -t \"resources/\" -o \"data.src\"");
	LOGR_MSG("\t-t : Target directory");
	LOGR_MSG("\t-o : Output file");
}

typedef struct
{
	FILE* outputFile;
	const char* targetDir;
	const char* outputFilePath;
	int packedFileCount;
} src_context;

static int StartPacking(src_context* ctx);
static int RecurseDirectory(src_context* ctx, const char* path);

int main(int argc, char** argv) {
	if (argc <= 1) {
		PrintUsage();
		return -1;
	}

	src_context ctx = {0};
	ctx.outputFilePath = "compiled.src";
	int handledArgs = 1;

	while (handledArgs < argc) {
		const char* arg = argv[handledArgs];
		if (arg[0] != '-' || handledArgs + 1 == argc) {
			PrintUsage();
			LOGF_MSG("Failed to handle \"%s\"", arg);
			return -1;
		}

		if (strcmp(arg, "-o") == 0) {
			ctx.outputFilePath = argv[handledArgs + 1];
			handledArgs += 2;
		}
		else if (strcmp(arg, "-t") == 0) {
			ctx.targetDir = argv[handledArgs + 1];
			handledArgs += 2;
		}
	}
	
	return StartPacking(&ctx);
}

#define WRITE_STRUCT(buf, file) fwrite(&buf, sizeof(buf), 1, file)
#define WRITE_DATA(buf, len, file) fwrite(buf, 1, len, file)

void src_header_init(SimpleResourceHeader* header)
{
	memset(header, 0, sizeof(SimpleResourceHeader));
	strcpy(header->header, SRC_RESOURCE_HEADER_VALUE);
	header->version = SRC_RESOURCE_VERSION;
}

void src_write_header(src_context* ctx, size_t resourceCount)
{
	SimpleResourceHeader header;
	src_header_init(&header);
	header.subResourceCount = resourceCount;
	WRITE_STRUCT(header, ctx->outputFile);
}

int StartPacking(src_context* ctx)
{
	if (ctx->outputFile = fopen(ctx->outputFilePath, "wb")) {
		// write blank header
		src_write_header(ctx, 0);

		// write sub resources
		int succ = RecurseDirectory(ctx, ctx->targetDir);
		
		// update header
		fseek(ctx->outputFile, 0, SEEK_SET);
		src_write_header(ctx, ctx->packedFileCount);

		fclose(ctx->outputFile); ctx->outputFile = NULL;
		LOGF_MSG("Packaged %d files", ctx->packedFileCount);
		return succ;
	}
	else {
		LOGF_MSG("Failed to open output file \"%s\"", ctx->outputFilePath);
		return -1;
	}
	return 0;
}

int src_pack_file(src_context* ctx, cf_file_t* file)
{
	char buffer[1024];
	FILE* fileHandle = fopen(file->path, "rb");
	if (!fileHandle) return 0;

	SimpleResourceSubResourceHeader header = { 0 };
	strcpy(header.header, SRC_SUB_RESOURCE_HEADER_VALUE);
	LOGF_MSG("Hashing: \"%s\"", file->path);
	header.id = djb2_hash(file->path);
	LOGF_MSG("Id: %u", header.id);
	header.resourceSize = file->size;
	header.nameLen = strlen(file->path) + 1; // add null terminator
	header.flags = 0;
	WRITE_STRUCT(header, ctx->outputFile);
	WRITE_DATA(file->path, header.nameLen, ctx->outputFile);

	size_t bytesToWrite = header.resourceSize;
	while (bytesToWrite != 0) {
		size_t read = fread(buffer, 1, sizeof(buffer), fileHandle);
		WRITE_DATA(buffer, read, ctx->outputFile);
		bytesToWrite -= read;
	}

	fclose(fileHandle);
	ctx->packedFileCount += 1;
	return 1;
}

int RecurseDirectory(src_context* ctx, const char* path)
{
	cf_dir_t dir;
	if (!cf_dir_open(&dir, path)) {
		LOGF_MSG("Failed to open \"%s\"", path);
		return -1;
	}

	while (dir.has_next) {
		cf_file_t file;
		cf_read_file(&dir, &file);
		if (file.is_dir && file.name[0] != '.') {
			// recurse
			LOGF_MSG("Found directory: \"%s\"", file.name);
			RecurseDirectory(ctx, file.path);
		}
		else if (!file.is_dir && file.is_reg) {
			// pack resource
			LOGF_MSG("Packing: \"%s\"", file.name);
			if (!src_pack_file(ctx, &file)) {
				LOGF_MSG("Failed to pack file: \"%s\"", file.path);
				return -1;
			}
		}

		cf_dir_next(&dir);
	}
	cf_dir_close(&dir);
	return 0;
}