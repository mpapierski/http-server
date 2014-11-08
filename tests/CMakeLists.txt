add_library (tap
    tap/tap.c
    tap/tap.h)
include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/tap/)

add_executable (test_http_server
    test_http_server.c)
target_link_libraries (test_http_server
    http_server
    tap)
add_test (http_server
    test_http_server)
