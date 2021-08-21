// HACK: dont do this with format string
//       move this to another function instead
static const char* src_helper_definitions = 
    "const char* src_get_%s_resource_name(int32_t id);\n"
    "size_t src_get_%s_resource_offset(int32_t id);\n"
    "";

static const char* src_helper_impl = 
    "const char* src_get_%s_resource_name(int32_t id) {\n"
    "\t" "const char* name = %s_RESOURCE_NAMES[id];\n"
    "\t" "return name;\n"
    "}\n\n"

    "size_t src_get_%s_resource_offset(int32_t id) {\n"
    "\t" "size_t offset = %s_RESOURCE_OFFSETS[id];\n"
    "\t" "return offset;\n"
    "}\n\n";