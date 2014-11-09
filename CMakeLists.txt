cmake_minimum_required (VERSION 2.6)
project (http_server C)

option (HTTP_SERVER_TESTS "Build http-server tests" OFF)
option (HTTP_SERVER_EXAMPLES "Build http-server examples" OFF)

SET (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

add_subdirectory (deps)
include_directories (
    include/
    deps/http-parser/)

add_subdirectory (src)

if (HTTP_SERVER_TESTS)
    enable_testing ()
    add_subdirectory (tests)
endif (HTTP_SERVER_TESTS)

if (HTTP_SERVER_EXAMPLES)
	add_subdirectory (examples)
endif ()