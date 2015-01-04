set (HTTP_SERVER_SOURCES
    server.c
    errors.c
    client.c
    response.c
    event.c
    handler.c
    string.c)
	
set (HTTP_SERVER_HEADERS
	event.h)

# Check for availability of platform-dependent functions
include (CheckFunctionExists)
Check_Function_Exists(select HTTP_SERVER_HAVE_SELECT)
if (HTTP_SERVER_HAVE_SELECT)
	list (APPEND HTTP_SERVER_SOURCES
		event_select.c)
endif ()

Check_Function_Exists (kqueue HTTP_SERVER_HAVE_KQUEUE)
if (HTTP_SERVER_HAVE_KQUEUE)
	list (APPEND HTTP_SERVER_SOURCES
		event_kqueue.c)
endif ()

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/build_config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/build_config.h)

include_directories (${CMAKE_CURRENT_BINARY_DIR})

add_library (http_server
	${HTTP_SERVER_SOURCES}
	${HTTP_SERVER_HEADERS})

target_link_libraries (http_server
	http_parser)
