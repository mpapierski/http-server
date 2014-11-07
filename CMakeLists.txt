cmake_minimum_required (VERSION 2.6)
project (http_server C)

option (HTTP_SERVER_TESTS "Build http-server tests" OFF)

include_directories (
    include/)

add_subdirectory (src)

if (HTTP_SERVER_TESTS)
    enable_testing ()
    add_subdirectory (tests)
endif (HTTP_SERVER_TESTS)
