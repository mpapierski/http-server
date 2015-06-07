#include "clar.h"
#include "http-server/http-server.h"

http_server server;
http_server_handler handler;
http_server_client * client = NULL;

void test_client__initialize(void)
{
	client = http_server_new_client(&server, HTTP_SERVER_INVALID_SOCKET, &handler);
	client->parser_.method = HTTP_POST;
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

	unsigned int status;
	r = http_server_client_getinfo(client, HTTP_SERVER_CLIENTINFO_METHOD, &status);
	cl_assert_equal_i(r, HTTP_SERVER_OK);
	cl_assert_equal_i(status, HTTP_POST);

}

void test_client__write(void)
{
	cl_assert(TAILQ_EMPTY(&client->buffer));
	cl_assert_equal_i(http_server_client_write(client, "Hello", 5), HTTP_SERVER_OK);
	cl_assert(!TAILQ_EMPTY(&client->buffer));
	int i = 0;
	http_server_buf * buf;
	TAILQ_FOREACH(buf, &client->buffer, bufs)
	{
		i++;
	}
	cl_assert_equal_i(i, 1);
}
