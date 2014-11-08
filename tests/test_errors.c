#include "tap.h"
#include "http-server/http-server.h"

int main(int argc, char * argv[])
{
    plan(4);
    cmp_ok((long)http_server_errstr(123456789), "==", 0);
    is(http_server_errstr(HTTP_SERVER_OK), "Success");
    is(http_server_errstr(HTTP_SERVER_SOCKET_ERROR), "Invalid socket");
    is(http_server_errstr(HTTP_SERVER_NOTIMPL), "Not implemented error");
    return 0;
}