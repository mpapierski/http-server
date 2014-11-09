add_library (http_server
    server.c
    errors.c
    client.c)
target_link_libraries (http_server
	http_parser)
