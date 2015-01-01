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

find_package (Boost COMPONENTS
	system
	thread)
if (Boost_FOUND)
	include_directories (${Boost_INCLUDE_DIRS})
	add_executable (asio
		asio.cc)
	target_link_libraries (asio
		http_server
		${Boost_LIBRARIES})
endif ()
