#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>          /* hostent struct, gethostbyname() */
#include <arpa/inet.h>      /* inet_ntoa() to format IP address */
#include <netinet/in.h>     /* in_addr structure */
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>

typedef struct
{
    // sockets for reading
    fd_set rdset;
    // sockets for writing
    fd_set wrset;
    // maximum descriptor from all the sockets
    int nsock;
} default_event_handler;

static int _default_opensocket_function(void * clientp)
{
    int s;
    int optval;
    // create default ipv4 socket for listener
    s = socket(AF_INET, SOCK_STREAM, 0);
    fprintf(stderr, "open socket: %d\n", s);
    if (s == -1)
    {
        return HTTP_SERVER_INVALID_SOCKET;
    }
    // TODO: this part should be separated somehow
    // set SO_REUSEADDR on a socket to true (1):
    optval = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1)
    {
        perror("setsockopt");
        return HTTP_SERVER_INVALID_SOCKET;
    }

    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in sin;
    sin.sin_port = htons(5000);
    sin.sin_addr.s_addr = 0;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_family = AF_INET;

    if (bind(s, (struct sockaddr *)&sin,sizeof(struct sockaddr_in) ) == -1)
    {
        perror("bind");
        return HTTP_SERVER_INVALID_SOCKET;
    }

    if (listen(s, 128) < 0) {
        perror("listen");
        return HTTP_SERVER_INVALID_SOCKET;
    }
    return s;
}

static int _default_socket_function(void * clientp, http_server_socket_t sock, int flags, void * socketp)
{
    // Data assigned to listening socket is a `fd_set` 
    http_server * srv = clientp;
    default_event_handler * ev = socketp;
    int r;
    if (!ev)
    {
        fprintf(stderr, "new event handler\n");
        // Create empty event handler
        ev = malloc(sizeof(default_event_handler));
        r = http_server_assign(srv, sock, ev);
        assert(r == HTTP_SERVER_OK);
        FD_ZERO(&ev->rdset);
        FD_ZERO(&ev->wrset);
        ev->nsock = 0;
    }
    assert(ev);
    fprintf(stderr, "sock: %d\n", sock);
    // POLL_IN means "check for read ability"
    if (flags & HTTP_SERVER_POLL_IN)
    {
        FD_SET(sock, &ev->rdset);
        if (sock > ev->nsock)
        {
            ev->nsock = sock;
        }
    }
    return HTTP_SERVER_OK;
}

int http_server_init(http_server * srv)
{
    srv->sock_listen = HTTP_SERVER_INVALID_SOCKET;
    srv->sock_listen_data = NULL;
    srv->opensocket_func = &_default_opensocket_function;
    srv->opensocket_data = srv;
    srv->socket_func = &_default_socket_function;
    srv->socket_data = srv;
    SLIST_INIT(&srv->clients);
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

int http_server_start(http_server * srv)
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

int http_server_run(http_server * srv)
{
    int r;
    default_event_handler * ev = srv->sock_listen_data;
    do
    {
        fd_set rd, wr;
        FD_COPY(&ev->rdset, &rd);
        FD_COPY(&ev->wrset, &wr);
        // This will block
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        fprintf(stderr, "select(%d, {%d}, ...)\n", ev->nsock + 1, srv->sock_listen);
        r = select(ev->nsock + 1, &rd, &wr, 0, &tv);
        if (r == -1)
        {
            perror("select");
        }
        else
        {
            fprintf(stderr, "select result=%d\n", r);
        }
        if (r == 0)
        {
            // Nothing to do...
            continue;
        }
        if (FD_ISSET(srv->sock_listen, &ev->rdset))
        {
            // Check for new connection
            // This will create new client structure and it will be
            // saved in list.
            struct sockaddr_in cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            // accept
            FD_CLR(srv->sock_listen, &ev->rdset);
            fprintf(stderr, "new conn\n");
            int fd = accept(srv->sock_listen, (struct sockaddr *) &cli_addr, &clilen);
            if (fd == -1)
            {
                perror("accept");
            }
            // Add this socket to managed list
            if (http_server_add_client(srv, fd) != HTTP_SERVER_OK)
            {
                // If we can't manage this socket then disconnect it.
                close(fd);
            }
            if (srv->socket_func(srv->socket_data, srv->sock_listen, HTTP_SERVER_POLL_IN, srv->sock_listen_data) != HTTP_SERVER_OK)
            {
                close(fd);
            }
            continue;
        }
    }
    while (r != -1);
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

int http_server_add_client(http_server * srv, http_server_socket_t sock)
{
    assert(srv);
    if (sock == srv->sock_listen)
    {
        return HTTP_SERVER_SOCKET_EXISTS;
    }
    // Check if client exists on the list
    http_server_client * it = NULL;
    SLIST_FOREACH(it, &srv->clients, next)
    {
        assert(it);
        if (it->sock == sock)
        {
            return HTTP_SERVER_SOCKET_EXISTS;
        }
    }
    it = malloc(sizeof(http_server_client));
    it->sock = sock;
    it->data = NULL;
    SLIST_INSERT_HEAD(&srv->clients, it, next);   
    return HTTP_SERVER_OK;    
}

int http_server_pop_client(http_server * srv, http_server_socket_t sock)
{
    assert(srv);
    int r = HTTP_SERVER_INVALID_SOCKET;
    http_server_client * it = NULL;
    SLIST_FOREACH(it, &srv->clients, next)
    {
        assert(it);
        if (it->sock == sock)
        {
            SLIST_REMOVE(&srv->clients, it, http_server_client, next);
            // TODO: close socket callback
            close(it->sock);
            free(it);
            r = HTTP_SERVER_OK;
            break;
        }
    }
    return r;
}
