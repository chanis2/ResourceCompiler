project(src C)

set(src_SOURCES
  "simple_recource_compiler.c"
)

add_executable(${PROJECT_NAME} ${src_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC 
	${PROJECT_SOURCE_DIR}
)

add_library(SimpleResourceCompilerHeader INTERFACE)
target_include_directories(SimpleResourceCompilerHeader INTERFACE 
	${PROJECT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
	src_HeaderOnlyLibs
)

if(WIN32)
	set_property(TARGET ${PROJECT_NAME} PROPERTY 
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(UNIX AND NOT APPLE) # clang/gcc

endif()