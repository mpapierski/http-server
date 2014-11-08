#include <assert.h>
#include "http-server/http-server.h"

char * http_server_errstr(http_server_errno e)
{
    switch (e)
    {
    case HTTP_SERVER_OK:
        return "Success";
    case HTTP_SERVER_SOCKET_ERROR:
        return "Invalid socket";
    case HTTP_SERVER_NOTIMPL:
        return "Not implemented error";
    default:
        // Invalid error code
        return 0;
    }
}
