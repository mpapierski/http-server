#include "clar.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "http-server/http-server.h"
#include <sys/socket.h>
#include <unistd.h>

static http_server srv;

void test_test_http_server__initialize(void)
{
    int result = http_server_init(&srv);
    cl_assert_equal_i(result, HTTP_SERVER_OK);

}

void test_test_http_server__cleanup(void)
{
    http_server_free(&srv);
}

static http_server_socket_t _opensocket_function(void * arg)
{
    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    cl_assert(s != -1);
    return s;
}

static int _closesocket_function(http_server_socket_t sock, void * arg)
{
    int result = close(sock);
    cl_assert(result != -1);
    return HTTP_SERVER_OK;
}

static int _socket_function(void * clientp, http_server_socket_t sock, int flags, void * socketp)
{
    return HTTP_SERVER_OK;
}

static int _debug_function(int kind, char * data, int length, void * userp)
{
    return HTTP_SERVER_OK;
}

void test_test_http_server__setopt(void)
{
    int r;
    int opensocket_data = 42;
    int closesocket_data = 43;
    int socket_data = 44;
    int debug_data = 45;

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_DATA, &opensocket_data);
    cl_assert(r == HTTP_SERVER_OK);
    cl_assert(srv.opensocket_data == &opensocket_data);
    
    r = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_FUNCTION, &_opensocket_function);
    cl_assert(r == HTTP_SERVER_OK);
    cl_assert(srv.opensocket_func == &_opensocket_function);

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_CLOSE_SOCKET_DATA, &closesocket_data);
    cl_assert(r == HTTP_SERVER_OK);
    cl_assert(srv.closesocket_data == &closesocket_data);

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_CLOSE_SOCKET_FUNCTION, &_closesocket_function);
    cl_assert(r == HTTP_SERVER_OK);
    cl_assert(srv.opensocket_func == &_opensocket_function);

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_SOCKET_DATA, &socket_data);
    cl_assert(r == HTTP_SERVER_OK);
    cl_assert(srv.socket_data == &socket_data);
    
    r = http_server_setopt(&srv, HTTP_SERVER_OPT_SOCKET_FUNCTION, &_socket_function);
    cl_assert(r == HTTP_SERVER_OK);
    cl_assert(srv.socket_func == &_socket_function);

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_DEBUG_FUNCTION, &_debug_function);
    cl_assert_equal_i(r, HTTP_SERVER_OK);
    cl_assert(srv.debug_func == &_debug_function);

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_DEBUG_DATA, &debug_data);
    cl_assert_equal_i(r, HTTP_SERVER_OK);
    cl_assert(srv.debug_data == &debug_data);
}

void test_test_http_server__setopt_failure(void)
{
    int r;
    r = http_server_setopt(&srv, -1223424, 0);
    cl_assert_equal_i(r, HTTP_SERVER_INVALID_PARAM);
}

void test_test_http_server__start(void)
{
    int r;
    r = http_server_start(&srv);
    cl_assert_equal_i(r, HTTP_SERVER_OK);
    cl_assert(srv.sock_listen != HTTP_SERVER_INVALID_SOCKET);
}

void test_test_http_server__manage_clients(void)
{
    int r;
    
    r = http_server_add_client(&srv, 100);
    cl_assert_equal_i(r, HTTP_SERVER_OK);

    r = http_server_add_client(&srv, 200);
    cl_assert_equal_i(r, HTTP_SERVER_OK);

    r = http_server_add_client(&srv, 300);
    cl_assert_equal_i(r, HTTP_SERVER_OK);

    r = http_server_add_client(&srv, 300);
    cl_assert_equal_i(r, HTTP_SERVER_SOCKET_EXISTS);

    r = http_server_pop_client(&srv, 300);
    cl_assert_equal_i(r, HTTP_SERVER_OK);

    r = http_server_add_client(&srv, 300);
    cl_assert_equal_i(r, HTTP_SERVER_OK);

    r = http_server_pop_client(&srv, 300);
    cl_assert_equal_i(r, HTTP_SERVER_OK);

    r = http_server_pop_client(&srv, 300);
    cl_assert_equal_i(r, HTTP_SERVER_INVALID_SOCKET);
}