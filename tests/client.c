#include "clar.h"
#include "http-server/http-server.h"

http_server server;
http_server_handler handler;
http_server_client * client = NULL;

void test_client__initialize(void)
{
	client = http_server_new_client(&server, HTTP_SERVER_INVALID_SOCKET, &handler);
}

void test_client__cleanup(void)
{
	http_server_client_free(client);
}

void test_client__getinfo_empty(void)
{
	char * url;
	int r = http_server_client_getinfo(client, HTTP_SERVER_CLIENTINFO_URL, &url);
	cl_assert_equal_i(r, HTTP_SERVER_OK);
	cl_assert(!url);
}

void test_client__getinfo(void)
{
	int r = http_server_string_append(&client->url, "/g", 2);
	cl_assert_equal_i(r, HTTP_SERVER_OK);
	r = http_server_string_append(&client->url, "et/", 3);
	cl_assert_equal_i(r, HTTP_SERVER_OK);
	char * url;
	r = http_server_client_getinfo(client, HTTP_SERVER_CLIENTINFO_URL, &url);
	cl_assert_equal_i(r, HTTP_SERVER_OK);
	cl_assert(url);
	cl_assert_equal_s(url, "/get/");
}
