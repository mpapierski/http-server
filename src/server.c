#include "http-server/http-server.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>

static int _default_opensocket_function(void * clientp)
{
    int s;
    // create default ipv4 socket for listener
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        return HTTP_SERVER_INVALID_SOCKET;
    }
    return s;
}

int http_server_init(http_server * srv)
{
    srv->opensocket_func = &_default_opensocket_function;
    srv->opensocket_data = srv;
    return HTTP_SERVER_OK;
}

/**
 * Cleans up HTTP server instance private fields.
 */
void http_server_free(http_server * srv)
{
    
}

int http_server_setopt(http_server * srv, http_server_option opt, ...)
{
    va_list ap;
    va_start(ap, opt);
    if (opt >= HTTP_SERVER_POINTER_POINT)
    {
        void * ptr = va_arg(ap, void*);
        if (opt == HTTP_SERVER_OPT_OPEN_SOCKET_DATA)
        {
            srv->opensocket_data = ptr;
            fprintf(stderr, "set opensocket data %p\n", ptr);
        }
    }
    else if (opt >= HTTP_SERVER_FUNCTION_POINT)
    {
        if (opt == HTTP_SERVER_OPT_OPEN_SOCKET_FUNCTION)
        {
            srv->opensocket_func = va_arg(ap, http_server_opensocket_callback);
            fprintf(stderr, "set opensocket func\n");
        }
    }
    va_end(ap);
    return HTTP_SERVER_OK;
}
