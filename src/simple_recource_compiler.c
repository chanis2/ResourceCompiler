#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define CUTE_FILES_IMPLEMENTATION
#include "cute_files.h"

#define SIMPLE_RESOURCE_COMPILER_IMPLEMENTATION
#include "simple_resource_compiler.h"

#ifdef WIN32
const char PrefPathDelimiter = '\\';
const char OtherPathDelimiter = '/';
#else
const char PathDelimiter = '/';
const char OtherPathDelimiter = '\\';
#endif

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
	// the input directory
	const char* targetDir;
	
	// the file generated
	const char* outputFilePath;
	const char* outputFileName;
	const char* uppercaseFilename;
	FILE* outputFile;

	// the header file generated
	const char* outputHeaderPath;
	FILE* outputHeaderFile;

	// temporary files to assemble header
	FILE* tmpStringTableFile;
	FILE* tmpIdTableFile;

	int packedFileCount;
} src_context;

char FormatBuffer[1024];

static int StartPacking(src_context* ctx);
static int RecurseDirectory(src_context* ctx, const char* path);
static char* GetFilename(const char* path, int withExtension);
static char* ToUppercase(char* text);
static char* SanitizeName(char* name);

static void CopyFileToFile(FILE* dst, FILE* src, size_t bytesToCopy);

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
			size_t headerPathLen = strlen(ctx.outputFilePath) + 16;
			ctx.outputFileName = GetFilename(ctx.outputFilePath, FALSE);
			ctx.uppercaseFilename = ToUppercase(strdup(ctx.outputFileName));

			ctx.outputHeaderPath = (char*)malloc(headerPathLen);
			snprintf((char*)ctx.outputHeaderPath, headerPathLen, "%s.h", ctx.outputFilePath);
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

#define WRITE_TEXT(text, file) fwrite(text, 1, strlen(text), file)

#define TMP_FORMAT(fmt, ...) format_helper(FormatBuffer, sizeof(FormatBuffer), fmt, __VA_ARGS__)

static const char* format_helper(char* buf, uint32_t bufSize, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, bufSize, fmt, args);
	va_end(args);
	return buf;
}

static void src_header_init(SimpleResourceHeader* header)
{
	memset(header, 0, sizeof(SimpleResourceHeader));
	strcpy(header->header, SRC_RESOURCE_HEADER_VALUE);
	header->version = SRC_RESOURCE_VERSION;
}

static void src_write_header(src_context* ctx, size_t resourceCount)
{
	SimpleResourceHeader header;
	src_header_init(&header);
	header.subResourceCount = resourceCount;
	WRITE_STRUCT(header, ctx->outputFile);
}

static void src_write_header_start(src_context* ctx)
{
	WRITE_TEXT("// Automatically generated header with \"https://github.com/chanis2/ResourceCompiler\"\n", ctx->outputHeaderFile);
	WRITE_TEXT("// Do not edit.\n", ctx->outputHeaderFile);
	
	WRITE_TEXT("// =================================================================================\n", ctx->outputHeaderFile);
	time_t t = time(0);
	struct tm* tm = localtime(&t);
	WRITE_TEXT(TMP_FORMAT("//\t\tDate: %02d.%02d.%d\n",
		tm->tm_mday,
		tm->tm_mon + 1,
		tm->tm_year + 1900), ctx->outputHeaderFile);
	WRITE_TEXT(TMP_FORMAT("//\t\tTime: %02d:%02d:%02d\n",
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec), ctx->outputHeaderFile);
	WRITE_TEXT("// =================================================================================\n", ctx->outputHeaderFile);
	WRITE_TEXT("//\t\tInclude like this.\n", ctx->outputHeaderFile);
	WRITE_TEXT(TMP_FORMAT("//\t\t#define SRC_RESOURCE_%s_IMPLEMENTATION\n", ctx->uppercaseFilename), ctx->outputHeaderFile);
	WRITE_TEXT(TMP_FORMAT("//\t\t#include \"%s\"\n", ctx->outputHeaderPath), ctx->outputHeaderFile);
	WRITE_TEXT("// =================================================================================\n\n", ctx->outputHeaderFile);
	
	WRITE_TEXT(TMP_FORMAT("#ifndef SRC_RESOURCE_%s_HEADER\n", ctx->uppercaseFilename), ctx->outputHeaderFile);
	WRITE_TEXT(TMP_FORMAT("#define SRC_RESOURCE_%s_HEADER\n", ctx->uppercaseFilename), ctx->outputHeaderFile);
	WRITE_TEXT("#include \"simple_resource_compiler.h\"\n\n", ctx->outputHeaderFile);

	WRITE_TEXT("#ifdef __cplusplus\n", ctx->outputHeaderFile);
	WRITE_TEXT("extern \"C\" {\n", ctx->outputHeaderFile);
	WRITE_TEXT("#endif\n\n", ctx->outputHeaderFile);

	WRITE_TEXT(TMP_FORMAT("#ifdef SRC_RESOURCE_%s_IMPLEMENTATION\n", ctx->uppercaseFilename), ctx->outputHeaderFile);


	WRITE_TEXT(TMP_FORMAT("char* %s_ResourceNames[] = {\n", ctx->outputFileName), ctx->tmpStringTableFile);
	WRITE_TEXT(TMP_FORMAT("enum SRC_RESOURCE_%s_ID : int32_t {\n", ctx->uppercaseFilename), ctx->tmpIdTableFile);
}

static int src_pack_file(src_context* ctx, cf_file_t* file)
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

#if 1
	LOGF_MSG("Copying file!!!!!!!!!!", "");
	CopyFileToFile(ctx->outputFile, fileHandle, header.resourceSize);
#endif

	fclose(fileHandle);

	// write name table
	WRITE_TEXT(TMP_FORMAT("\t\"%s\",\n", file->path), ctx->tmpStringTableFile);

	char* resourceName = SanitizeName(ToUppercase(GetFilename(file->path, TRUE)));
	WRITE_TEXT(
		TMP_FORMAT(
			"\tSRC_%s_%s = %d,\n",
			ctx->uppercaseFilename,
			resourceName,
			ctx->packedFileCount),
		ctx->tmpIdTableFile);
	free(resourceName);
	ctx->packedFileCount += 1;
	return 1;
}

static void src_write_header_end(src_context* ctx)
{
	WRITE_TEXT("};\n", ctx->tmpStringTableFile);
	WRITE_TEXT("};\n", ctx->tmpIdTableFile);

	fseek(ctx->tmpStringTableFile, 0, SEEK_END);
	size_t tmpFileSize = ftell(ctx->tmpStringTableFile);
	rewind(ctx->tmpStringTableFile);
	CopyFileToFile(ctx->outputHeaderFile, ctx->tmpStringTableFile, tmpFileSize);

	fseek(ctx->tmpIdTableFile, 0, SEEK_END);
	tmpFileSize = ftell(ctx->tmpIdTableFile);
	rewind(ctx->tmpIdTableFile);
	CopyFileToFile(ctx->outputHeaderFile, ctx->tmpIdTableFile, tmpFileSize);

	WRITE_TEXT(TMP_FORMAT("\n#endif // SRC_RESOURCE_%s_IMPLEMENTATION\n", ctx->uppercaseFilename), ctx->outputHeaderFile);

	WRITE_TEXT("#ifdef __cplusplus\n", ctx->outputHeaderFile);
	WRITE_TEXT("} // extern \"C\"\n", ctx->outputHeaderFile);
	WRITE_TEXT("#endif\n", ctx->outputHeaderFile);

	WRITE_TEXT(TMP_FORMAT("#endif // SRC_RESOURCE_%s_HEADER\n", ctx->uppercaseFilename), ctx->outputHeaderFile);
}

static int StartPacking(src_context* ctx)
{
	if (ctx->outputFile = fopen(ctx->outputFilePath, "wb")) {
		ctx->outputHeaderFile = fopen(ctx->outputHeaderPath, "w");
		ctx->tmpStringTableFile = fopen("stringTable.tmp", "w+");
		ctx->tmpIdTableFile = fopen("idTable.tmp", "w+");

		if (!ctx->outputHeaderFile || !ctx->tmpStringTableFile) {
			if(ctx->outputFile) fclose(ctx->outputFile);
			if(ctx->tmpStringTableFile) fclose(ctx->tmpStringTableFile);
			if(ctx->tmpIdTableFile) fclose(ctx->tmpIdTableFile);
			LOGF_MSG("Failed to open header file.");
			return -1;
		}

		// write header start
		src_write_header_start(ctx);

		// write blank header
		src_write_header(ctx, 0);

		// write sub resources
		int succ = RecurseDirectory(ctx, ctx->targetDir);
		
		// update header
		fseek(ctx->outputFile, 0, SEEK_SET);
		src_write_header(ctx, ctx->packedFileCount);

		// write header end
		src_write_header_end(ctx);

		fclose(ctx->outputHeaderFile); ctx->outputHeaderFile = NULL;
		fclose(ctx->outputFile); ctx->outputFile = NULL;
		fclose(ctx->tmpStringTableFile); ctx->tmpStringTableFile = NULL;
		fclose(ctx->tmpIdTableFile); ctx->tmpIdTableFile = NULL;

		LOGF_MSG("Packaged %d files", ctx->packedFileCount);
		return succ;
	}
	else {
		LOGF_MSG("Failed to open output file \"%s\"", ctx->outputFilePath);
		return -1;
	}
	return 0;
}

static int RecurseDirectory(src_context* ctx, const char* path)
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

static char* GetFilename(const char* path, int withExtension)
{
	// TODO: test this with actual paths
	int len = strlen(path);
	int i = len;
	int extPos = len;
	for (; i >= 0; i -= 1) {
		if (path[i] == '.') {
			extPos = i;
		}
		if (path[i] == PrefPathDelimiter 
			|| path[i] == OtherPathDelimiter) break;
	}
	i += 1;

	int extLen = (len - extPos) - 1;
	int nameLen = len - i;

	int actLen = nameLen;
	if(!withExtension) 
		actLen -= extLen + 1;

	char* filename = (char*)malloc(actLen + 1);
	strncpy(filename, path + i, actLen);
	filename[actLen] = '\0';
	return filename;
}

static char* ToUppercase(char* text)
{
	int i = 0;
	while (text[i] != '\0') {
		text[i] = toupper(text[i]);
		i += 1;
	}
	return text;
}

static void CopyFileToFile(FILE* dst, FILE* src, size_t bytesToCopy)
{
	char buffer[1024];
	while (bytesToCopy != 0) {
		size_t read = fread(buffer, 1, sizeof(buffer), src);
		assert(ferror(src) == 0);
		if (read == 0) break;
		WRITE_DATA(buffer, read, dst);
		bytesToCopy -= read;
	}
}

static char* SanitizeName(char* name)
{
	int i = 0;
	while (name[i] != '\0') {
		if (name[i] == '-' 
			|| name[i] == '.') {		
			name[i] = '_';
		}
		else if (name[i] == '('
			|| name[i] == ')') {
			name[i] = ' ';
		}

		i += 1;
	}

	// remove spaces
	i = 0;
	int offset = 0;
	while (name[i] != '\0') {
		while (name[i + offset] == ' ' 
			&& name[i + offset] != '\0') {
			offset += 1;
		}
		name[i] = name[i + offset];
		i += 1;
	}

	return name;
}