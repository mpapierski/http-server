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

static int _socket_function(void * clientp, http_server_socket_t sock, int flags, void * socketp)
{
    return HTTP_SERVER_OK;
}

void test_setopt()
{
    int r;
    int data = 42;

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_DATA, &data);
    ok(r == HTTP_SERVER_OK, "setopt (pointer) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.opensocket_data == &data, "setopt (open socket data) is correct");
    
    r = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_FUNCTION, &_opensocket_function);
    ok(r == HTTP_SERVER_OK, "setopt (function) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.opensocket_func == &_opensocket_function, "setopt (open socket function) is correct");

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_SOCKET_DATA, &data);
    ok(r == HTTP_SERVER_OK, "setopt (pointer) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.socket_data == &data, "setopt (socket data) is correct");
    
    r = http_server_setopt(&srv, HTTP_SERVER_OPT_SOCKET_FUNCTION, &_socket_function);
    ok(r == HTTP_SERVER_OK, "setopt (function) result %d (expected %d)", r, HTTP_SERVER_OK);
    ok(srv.socket_func == &_socket_function, "setopt (socket function) is correct");
}

void test_run()
{
    int r;
    r = http_server_run(&srv);
    cmp_ok(r, "==", HTTP_SERVER_OK);
    cmp_ok(srv.sock_listen, "!=", HTTP_SERVER_INVALID_SOCKET);
}

void test_cleanup()
{
    http_server_free(&srv);
}

int main(int argc, char * argv[])
{
    test_init();
    test_setopt();
    test_cleanup();
    test_run();
    done_testing();
    return 0;
}
