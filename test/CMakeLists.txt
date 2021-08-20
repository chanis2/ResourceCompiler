project(src_test CXX)

set(src_SOURCES
  "test.cpp"
)

add_executable(${PROJECT_NAME} ${src_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC 
	${PROJECT_SOURCE_DIR}
	"${CMAKE_SOURCE_DIR}/src/"
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
	stb
)

if(WIN32)
	set_property(TARGET ${PROJECT_NAME} PROPERTY 
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(UNIX AND NOT APPLE) # clang/gcc

endif()