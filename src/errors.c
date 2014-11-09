#include <assert.h>
#include "http-server/http-server.h"

char * http_server_errstr(http_server_errno e)
{
#define XX(id, value, descr) case HTTP_SERVER_ ## id: return descr;
    switch (e)
    {
    HTTP_SERVER_ENUM_ERROR_CODES(XX)
    default:
        // Invalid error code
        return 0;
    }
#undef XX
}
