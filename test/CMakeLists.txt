project(src_test CXX)

set(src_SOURCES
  "test.cpp"
)

add_executable(${PROJECT_NAME} ${src_SOURCES})

#add_test(NAME run_src
#	COMMAND src "-t" "../testData" "-o" "test.src"
#	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
#
#add_test(NAME run_src2
#	COMMAND src "-t" "../testData/font" "-o" "foo.src"
#	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
#set_tests_properties(run_src2 PROPERTIES DEPENDS "run_src;")

add_test(NAME src_check_result
	COMMAND ${PROJECT_NAME}
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
set_tests_properties(src_check_result PROPERTIES DEPENDS "run_src;run_src2;")


include(SimpleResourceCompiler)
src_compile_resources(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/testData/" "test.src")
src_compile_resources(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/testData/font/" "foo.src")
#src_compile_resources(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/testData/pic/" "pic.data")

target_include_directories(${PROJECT_NAME} PUBLIC 
	${PROJECT_SOURCE_DIR}
	"${CMAKE_SOURCE_DIR}/src/"
	${CMAKE_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
	stb
)

if(WIN32)
	set_property(TARGET ${PROJECT_NAME} PROPERTY 
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(UNIX AND NOT APPLE) # clang/gcc

endif()