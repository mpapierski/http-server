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
    // owner of the event handler
    http_server * srv;
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
    default_event_handler * ev = clientp;
    http_server * srv = ev->srv;
    assert(ev);
    fprintf(stderr, "sock: %d\n", sock);
    // POLL_IN means "check for read ability"
    if (flags & HTTP_SERVER_POLL_IN)
    {
        fprintf(stderr, "poll in fd=%d\n", sock);
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
    // Create new default event handler
    default_event_handler * ev = malloc(sizeof(default_event_handler));
    ev->srv = srv;
    FD_ZERO(&ev->rdset);
    FD_ZERO(&ev->wrset);
    ev->nsock = 0;
    srv->socket_data = ev;

    srv->sock_listen = HTTP_SERVER_INVALID_SOCKET;
    srv->sock_listen_data = ev;
    srv->opensocket_func = &_default_opensocket_function;
    srv->opensocket_data = srv;
    srv->socket_func = &_default_socket_function;

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
        
        // Check if client exists on the list
        http_server_client * it = NULL;
        SLIST_FOREACH(it, &srv->clients, next)
        {
            assert(it);
            if (FD_ISSET(it->sock, &rd))
            {
                fprintf(stderr, "client data is available fd=%d\n", it->sock);
                if (http_server_socket_action(srv, it->sock, HTTP_SERVER_POLL_IN) != HTTP_SERVER_OK)
                {
                    fprintf(stderr, "failed to do socket action on fd=%d\n", it->sock);
                    FD_CLR(it->sock, &ev->rdset);
                    close(it->sock);
                    http_server_pop_client(srv, it->sock);
                }
            }
        }

        if (FD_ISSET(srv->sock_listen, &rd))
        {
            // Check for new connection
            // This will create new client structure and it will be
            // saved in list.
            FD_CLR(srv->sock_listen, &ev->rdset);
            if (http_server_socket_action(srv, srv->sock_listen, HTTP_SERVER_POLL_IN) != HTTP_SERVER_OK)
            {
                fprintf(stderr, "unable to accept new client\n");
                continue;
            }
            continue;
        }
    }
    while (r != -1);
    return HTTP_SERVER_OK;
}

int http_server_assign(http_server * srv, http_server_socket_t sock, void * data)
{
    int r = HTTP_SERVER_OK;
    if (sock == srv->sock_listen)
    {
        srv->sock_listen_data = data;
        return r;
    }
    // Check if client exists on the list
    http_server_client * it = NULL;
    r = HTTP_SERVER_INVALID_PARAM;
    SLIST_FOREACH(it, &srv->clients, next)
    {
        assert(it);
        if (it->sock == sock)
        {
            it->data = data;
            r = HTTP_SERVER_OK;
            break;
        }
    }
    return r;
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
    it = http_server_new_client(sock);
    SLIST_INSERT_HEAD(&srv->clients, it, next);
    // Start polling for read
    int r = srv->socket_func(srv->socket_data, it->sock, HTTP_SERVER_POLL_IN, it->data);
    return r;
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

int http_server_socket_action(http_server * srv, http_server_socket_t socket, int flags)
{
    assert(srv);
    if (socket == srv->sock_listen)
    {
        // Listening socket is a special kind of socket to be managed
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        // accept
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
            return HTTP_SERVER_SOCKET_ERROR;
        }
        if (srv->socket_func(srv->socket_data, srv->sock_listen, HTTP_SERVER_POLL_IN, srv->sock_listen_data) != HTTP_SERVER_OK)
        {
            (void)http_server_pop_client(srv, fd);
            close(fd);
            return HTTP_SERVER_SOCKET_ERROR;
        }
        fprintf(stderr, "new client: %d\n", fd);
        return HTTP_SERVER_OK;
    }
    int r = HTTP_SERVER_OK;
    http_server_client * it, * client = NULL;
    SLIST_FOREACH(it, &srv->clients, next)
    {
        assert(it);
        if (it->sock == socket)
        {
            client = it;
            break;
        }
    }
    if (!client)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    if (flags & HTTP_SERVER_POLL_IN)
    {
        // Read data
        char tmp[1024];
        int bytes_received = read(client->sock, tmp, sizeof(tmp));
        if (bytes_received == -1)
        {
            r = HTTP_SERVER_SOCKET_ERROR;
        }
        else if (bytes_received == 0)
        {
            fprintf(stderr, "client eof %d\n", client->sock);
            r = HTTP_SERVER_CLIENT_EOF;
            if (srv->socket_func(srv->socket_data, it->sock, HTTP_SERVER_POLL_REMOVE, it->data) != HTTP_SERVER_OK)
            {
                return HTTP_SERVER_SOCKET_ERROR;
            }
            return r;
        }
        else
        {
            fprintf(stderr, "received %d bytes from %d\n", bytes_received, client->sock);
            if (http_server_perform_client(client, tmp, bytes_received) != HTTP_SERVER_OK)
            {
                // close connection
                if (srv->socket_func(srv->socket_data, it->sock, HTTP_SERVER_POLL_REMOVE, it->data) != HTTP_SERVER_OK)
                {
                    return HTTP_SERVER_SOCKET_ERROR;
                }
                close(it->sock);
            }
        }
    }
    return r;
}