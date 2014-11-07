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

void test_cleanup()
{
    http_server_free(&srv);
}

int main(int argc, char * argv[])
{
    test_init();
    test_cleanup();
    return 0;
}
