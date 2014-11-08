#include "http-server/http-server.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>

static int _default_opensocket_function(void * clientp)
{
    int s;
    // create default ipv4 socket for listener
    s = socket(AF_INET, SOCK_STREAM, 0);
    fprintf(stderr, "open socket: %d\n", s);
    if (s == -1)
    {
        return HTTP_SERVER_INVALID_SOCKET;
    }
    return s;
}

static int _default_socket_function(void * clientp, http_server_socket_t sock, int flags, void * socketp)
{
    return HTTP_SERVER_OK;
}

int http_server_init(http_server * srv)
{
    srv->sock_listen = HTTP_SERVER_INVALID_SOCKET;
    srv->opensocket_func = &_default_opensocket_function;
    srv->opensocket_data = srv;
    srv->socket_func = &_default_socket_function;
    srv->socket_data = NULL;
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
    fprintf(stderr, "setopt: %d\n", opt);
    if (opt >= HTTP_SERVER_POINTER_POINT && opt < HTTP_SERVER_FUNCTION_POINT)
    {
        void * ptr = va_arg(ap, void*);
        if (opt == HTTP_SERVER_OPT_OPEN_SOCKET_DATA)
        {
            srv->opensocket_data = ptr;
            fprintf(stderr, "set opensocket data %p\n", ptr);
        }
        if (opt == HTTP_SERVER_OPT_SOCKET_DATA)
        {
            srv->socket_data = ptr;
            fprintf(stderr, "set socket func data %p\n", ptr);
        }
        goto success;
    }
    if (opt >= HTTP_SERVER_FUNCTION_POINT)
    {
        if (opt == HTTP_SERVER_OPT_OPEN_SOCKET_FUNCTION)
        {
            srv->opensocket_func = va_arg(ap, http_server_opensocket_callback);
            fprintf(stderr, "set opensocket func\n");
        }
        if (opt == HTTP_SERVER_OPT_SOCKET_FUNCTION)
        {
            srv->socket_func = va_arg(ap, http_server_socket_callback);
            fprintf(stderr, "set socket func\n");
        }
        goto success;
    }
success:
    va_end(ap);
    return HTTP_SERVER_OK;
}

int http_server_run(http_server * srv)
{
    int r;
    // Create listening socket
    srv->sock_listen = srv->opensocket_func(srv->opensocket_data);
    if (srv->sock_listen == HTTP_SERVER_INVALID_SOCKET)
    {
        return HTTP_SERVER_SOCKET_ERROR;
    }
    // Start async poll
    srv->socket_func(srv->socket_data, srv->sock_listen, HTTP_SERVER_POLL_IN, srv->sock_listen_data);
    return HTTP_SERVER_OK;
}

int http_server_assign(http_server * srv, http_server_socket_t sock, void * data)
{
    if (sock == srv->sock_listen)
    {
        srv->sock_listen_data = data;
        return HTTP_SERVER_OK;
    }
    // TODO: Some data structore that holds pair of socket->data
    // where sockets is a some dynamic extendable list.
    return HTTP_SERVER_NOTIMPL;
}