cmake_minimum_required(VERSION 3.16)

project(SimpleResourceCompiler)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

# =============
# == OPTIONS ==
# =============
option(SRC_TESTING OFF)

# ====================
# === DEPENDENCIES ===
# ====================
add_subdirectory("lib/")

# ==============
# ==== SRC ====
# ==============
add_subdirectory("src/")

# ==============
# ==== test ====
# ==============
if(SRC_TESTING)
    include(CTest)
    add_subdirectory("test/")
endif()