add_executable (simple
    simple.c)
target_link_libraries (simple
    http_server)

find_package (LibUV)
if (LIBUV_FOUND)
	include_directories (${LIBUV_INCLUDE_DIRS})
    add_executable (simple-uv
        simple-uv.c)
    target_link_libraries (simple-uv
        http_server
        ${LIBUV_LIBRARIES})
endif ()
