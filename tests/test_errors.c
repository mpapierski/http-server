#include "tap.h"
#include "http-server/http-server.h"

int main(int argc, char * argv[])
{
    cmp_ok((long)http_server_errstr(123456789), "==", 0);
    is(http_server_errstr(HTTP_SERVER_OK), "Success");
    is(http_server_errstr(HTTP_SERVER_SOCKET_ERROR), "Invalid socket");
    is(http_server_errstr(HTTP_SERVER_NOTIMPL), "Not implemented error");
    is(http_server_errstr(HTTP_SERVER_SOCKET_EXISTS), "Socket is already managed");
    is(http_server_errstr(HTTP_SERVER_INVALID_PARAM), "Invalid parameter");
    is(http_server_errstr(HTTP_SERVER_CLIENT_EOF), "End of file");
    is(http_server_errstr(HTTP_SERVER_PARSER_ERROR), "Unable to parse HTTP request");
    done_testing();
    return 0;
}
