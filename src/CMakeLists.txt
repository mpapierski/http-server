set (HTTP_SERVER_SOURCES
    server.c
    errors.c
    client.c
    response.c
    event.c)
	
set (HTTP_SERVER_HEADERS
	event.h)

# TODO: check for select(2), epoll(2) etc. and compile correct event handler
list (APPEND HTTP_SERVER_SOURCES
	event_select.c)

add_library (http_server
	${HTTP_SERVER_SOURCES}
	${HTTP_SERVER_HEADERS})

target_link_libraries (http_server
	http_parser)
