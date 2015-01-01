#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include "tap/tap.h"
#include "http-server/http-server.h"

static http_server srv;

void test_init()
{
    int result = http_server_init(&srv);
    ok(result == HTTP_SERVER_OK, "init result %d (expected %d)", result, HTTP_SERVER_OK);
}

static http_server_socket_t _opensocket_function(void * arg)
{
    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    cmp_ok(s, "!=", -1, "create new tcp socket");
    return s;
}

static int _closesocket_function(http_server_socket_t sock, void * arg)
{
    int result = close(sock);
    cmp_ok(result, "!=", -1, "close tcp socket");
    return HTTP_SERVER_OK;
}

static int _socket_function(void * clientp, http_server_socket_t sock, int flags, void * socketp)
{
    return HTTP_SERVER_OK;
}

void test_setopt()
{
    int r;
    int opensocket_data = 42;
    int closesocket_data = 43;
    int socket_data = 44;

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_DATA, &opensocket_data);
    ok(r == HTTP_SERVER_OK, "setopt (pointer) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.opensocket_data == &opensocket_data, "setopt (open socket data) is correct");
    
    r = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_FUNCTION, &_opensocket_function);
    ok(r == HTTP_SERVER_OK, "setopt (function) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.opensocket_func == &_opensocket_function, "setopt (open socket function) is correct");

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_CLOSE_SOCKET_DATA, &closesocket_data);
    ok(r == HTTP_SERVER_OK, "setopt (pointer) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.closesocket_data == &closesocket_data, "setopt (open socket data) is correct");

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_CLOSE_SOCKET_FUNCTION, &_closesocket_function);
    ok(r == HTTP_SERVER_OK, "setopt (function) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.opensocket_func == &_opensocket_function, "setopt (open socket function) is correct");

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_SOCKET_DATA, &socket_data);
    ok(r == HTTP_SERVER_OK, "setopt (pointer) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.socket_data == &socket_data, "setopt (socket data) is correct");
    
    r = http_server_setopt(&srv, HTTP_SERVER_OPT_SOCKET_FUNCTION, &_socket_function);
    ok(r == HTTP_SERVER_OK, "setopt (function) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.socket_func == &_socket_function, "setopt (socket function) is correct");
}

void test_start()
{
    int r;
    r = http_server_start(&srv);
    cmp_ok(r, "==", HTTP_SERVER_OK);
    cmp_ok(srv.sock_listen, "!=", HTTP_SERVER_INVALID_SOCKET);
}

void test_cleanup()
{
    http_server_free(&srv);
}

void test_manage_clients()
{
    int r;
    
    r = http_server_add_client(&srv, 100);
    cmp_ok(r, "==", HTTP_SERVER_OK);

    r = http_server_add_client(&srv, 200);
    cmp_ok(r, "==", HTTP_SERVER_OK);

    r = http_server_add_client(&srv, 300);
    cmp_ok(r, "==", HTTP_SERVER_OK);

    r = http_server_add_client(&srv, 300);
    cmp_ok(r, "==", HTTP_SERVER_SOCKET_EXISTS);

    r = http_server_pop_client(&srv, 300);
    cmp_ok(r, "==", HTTP_SERVER_OK);

    r = http_server_add_client(&srv, 300);
    cmp_ok(r, "==", HTTP_SERVER_OK);

    r = http_server_pop_client(&srv, 300);
    cmp_ok(r, "==", HTTP_SERVER_OK);

    r = http_server_pop_client(&srv, 300);
    cmp_ok(r, "==", HTTP_SERVER_INVALID_SOCKET);
}

int main(int argc, char * argv[])
{
    test_init();
    test_setopt();
    test_cleanup();
    test_start();
    test_manage_clients();
    done_testing();
    return 0;
}
