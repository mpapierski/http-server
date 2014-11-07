add_executable (test_http_server
	test_http_server.c)
target_link_libraries (test_http_server
	http_server)
add_test (http_server
	test_http_server)
