#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "http-server/http-server.h"

http_server srv;

void test_init()
{
    int result = http_server_init(&srv);
    if (result != HTTP_SERVER_OK) exit(1);
}

http_server_socket_t _opensocket_function(void * arg)
{
    return HTTP_SERVER_INVALID_SOCKET;
}

void test_setopt()
{
    int r;
    int data = 42;
    fprintf(stderr, "data: %p\n", &data);

    r = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_DATA, &data);
    if (r != HTTP_SERVER_OK) exit(1);
    if (srv.opensocket_data != &data) exit(1);
    
    r = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_FUNCTION, &_opensocket_function);
    if (r != HTTP_SERVER_OK) exit(1);
    if (srv.opensocket_func != &_opensocket_function) exit(1);
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
    return 0;
}
