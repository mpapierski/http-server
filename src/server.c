#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <strings.h>
#include <errno.h>
#include <sys/uio.h>
#include "event.h"
#include "build_config.h"

#ifndef SLIST_FOREACH_SAFE
#define SLIST_FOREACH_SAFE(var, head, field, tvar) \
for ((var) = SLIST_FIRST((head)); \
(var) && ((tvar) = SLIST_NEXT((var), field), 1); \
(var) = (tvar))
#endif

int http_server_init(http_server * srv)
{
    // Clear all fields. All of them is initialized in some way or another
    // but in case of failure it is easier to debug.
    srv->sock_listen = HTTP_SERVER_INVALID_SOCKET;
    srv->sock_listen_data = NULL;
    srv->opensocket_func = NULL;
    srv->opensocket_data = NULL;
    srv->closesocket_func = NULL;
    srv->closesocket_data = NULL;
    srv->socket_func = NULL;
    srv->socket_data = NULL;
    SLIST_INIT(&srv->clients);
    srv->handler_ = NULL;
    srv->response_ = NULL;
    srv->event_loop_ = NULL;
    srv->event_loop_data_ = NULL;
    // Initialize event loop by its name
    char * event_loop = getenv("HTTP_SERVER_EVENT_LOOP");
#if defined(HTTP_SERVER_HAVE_KQUEUE)
    if (!event_loop)
    {
        event_loop = "kqueue";
    }
#elif defined(HTTP_SERVER_HAVE_SELECT)
    if (!event_loop)
    {
        event_loop = "select";
    }
#endif
    if (!event_loop)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    int result;
    fprintf(stderr, "initialize event loop %s\n", event_loop);
    if ((result = Http_server_event_loop_init(srv, event_loop)) != HTTP_SERVER_OK)
    {
        return result;
    }
    return HTTP_SERVER_OK;
}

/**
 * Cleans up HTTP server instance private fields.
 */
void http_server_free(http_server * srv)
{
    Http_server_event_loop_free(srv);    
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
        if (opt == HTTP_SERVER_OPT_CLOSE_SOCKET_DATA)
        {
            srv->closesocket_data = ptr;
            fprintf(stderr, "set close socket func data %p\n", ptr);
        }
        if (opt == HTTP_SERVER_OPT_SOCKET_DATA)
        {
            srv->socket_data = ptr;
            fprintf(stderr, "set socket func data %p\n", ptr);
        }
        if (opt == HTTP_SERVER_OPT_HANDLER)
        {
            srv->handler_ = ptr;
            fprintf(stderr, "set handler %p\n", ptr);
        }
        if (opt == HTTP_SERVER_OPT_HANDLER_DATA)
        {
            srv->handler_->data = ptr;
            fprintf(stderr, "set handler data %p\n", ptr);
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
        if (opt == HTTP_SERVER_OPT_CLOSE_SOCKET_FUNCTION)
        {
            srv->closesocket_func = va_arg(ap, http_server_closesocket_callback);
            fprintf(stderr, "set closesocket func\n");
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
    assert(srv);
    assert(srv->sock_listen);
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

int http_server_cancel(http_server * srv)
{
    assert(srv);
    if (srv->sock_listen == HTTP_SERVER_INVALID_SOCKET)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    // Stop polling on this socket
   if (srv->socket_func(srv->socket_data, srv->sock_listen, HTTP_SERVER_POLL_REMOVE, srv->sock_listen_data) != HTTP_SERVER_OK)
    {
        return HTTP_SERVER_SOCKET_ERROR;
    }
    // Close acceptor
    if (srv->closesocket_func(srv->sock_listen, srv->closesocket_data) != HTTP_SERVER_OK)
    {
        return HTTP_SERVER_SOCKET_ERROR;
    }
    return HTTP_SERVER_OK;
}

int http_server_run(http_server * srv)
{
    return Http_server_event_loop_run(srv);
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
    it = http_server_new_client(srv, sock, srv->handler_);
    SLIST_INSERT_HEAD(&srv->clients, it, next);
    // Start polling for read
    int r = srv->socket_func(srv->socket_data, it->sock, HTTP_SERVER_POLL_IN, it->data);
    return r;
}

int http_server_pop_client(http_server * srv, http_server_socket_t sock)
{
    assert(srv);
    int r = HTTP_SERVER_INVALID_SOCKET;
    http_server_client * it, * it_temp;
    SLIST_FOREACH_SAFE(it, &srv->clients, next, it_temp)
    {
        assert(it);
        if (it->sock == sock)
        {
            SLIST_REMOVE(&srv->clients, it, http_server_client, next);
            http_server_client_free(it);
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
            return HTTP_SERVER_SOCKET_ERROR;
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
    http_server_client * it, * it_temp, * client = NULL;
    SLIST_FOREACH_SAFE(it, &srv->clients, next, it_temp)
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
        // Socket not found in managed list of clients.
        // Some event loops does accept(2) for you,
        // so in this case calling `http_server_socket_action`
        // with newly accepted client is fine.
        if (http_server_add_client(srv, socket) != HTTP_SERVER_OK)
        {
            // If we can't manage this socket then disconnect it.
            close(socket);
            return HTTP_SERVER_SOCKET_ERROR;
        }
        if (srv->socket_func(srv->socket_data, srv->sock_listen, HTTP_SERVER_POLL_IN, srv->sock_listen_data) != HTTP_SERVER_OK)
        {
            (void)http_server_pop_client(srv, socket);
            close(socket);
            return HTTP_SERVER_SOCKET_ERROR;
        }
        return HTTP_SERVER_OK;
    }
    if ((client->current_flags & flags) != 0)
    {
        client->current_flags ^= flags;
    }
    if (flags & HTTP_SERVER_POLL_IN)
    {
        // Read data
        char tmp[16384];
        int bytes_received = read(client->sock, tmp, sizeof(tmp));
        if (bytes_received == -1)
        {
            r = HTTP_SERVER_SOCKET_ERROR;
        }
        else if (bytes_received == 0)
        {
            fprintf(stderr, "client eof %d\n", client->sock);
            if (http_server_perform_client(client, tmp, bytes_received) != HTTP_SERVER_OK)
            {
                return HTTP_SERVER_SOCKET_ERROR;
            }
            if (http_server_poll_client(it, HTTP_SERVER_POLL_REMOVE) != HTTP_SERVER_OK)
            {
                return HTTP_SERVER_SOCKET_ERROR;
            }
            if (srv->closesocket_func(it->sock, srv->closesocket_data) != HTTP_SERVER_OK)
            {
                return HTTP_SERVER_SOCKET_ERROR;
            }
            // Remove client from the list and tell the caller that it should not
            // do any operation with current socket.
            SLIST_REMOVE(&srv->clients, it, http_server_client, next);
            free(it);
            return HTTP_SERVER_CLIENT_EOF;
        }
        else
        {
            fprintf(stderr, "received %d bytes from %d\n", bytes_received, client->sock);
            if (http_server_perform_client(client, tmp, bytes_received) != HTTP_SERVER_OK)
            {
                // TODO: close connection for now but this should be something like 400 BAD REQUEST.
                if (http_server_poll_client(it, HTTP_SERVER_POLL_REMOVE) != HTTP_SERVER_OK)
                {
                    return HTTP_SERVER_SOCKET_ERROR;
                }
                if (srv->closesocket_func(it->sock, srv->closesocket_data) != HTTP_SERVER_OK)
                {
                    return HTTP_SERVER_SOCKET_ERROR;
                }
                return r;
            }
            fprintf(stderr, "is_complete: %d\n", it->is_complete);
            if (!it->is_complete && http_server_poll_client(it, HTTP_SERVER_POLL_IN) != HTTP_SERVER_OK)
            {
                fprintf(stderr, "unable to poll in - request incomplete\n");
                return HTTP_SERVER_SOCKET_ERROR;
            }
        }
    }
    if (flags & HTTP_SERVER_POLL_OUT)
    {
        assert(!TAILQ_EMPTY(&it->buffer));
        // Use scatter-gather I/O to deliver multiple chunks of data
        static const int maxiov =
#if defined(MAXIOV)
            MAXIOV
#elif defined(IOV_MAX)
            IOV_MAX
#else
            8192
#endif
            ;
        struct iovec wvec[maxiov];
        http_server_buf * buf;
        int iocnt = 0;
        TAILQ_FOREACH(buf, &client->buffer, bufs)
        {
            assert(buf);
            if (iocnt >= maxiov)
            {
                break;
            }
            wvec[iocnt].iov_base = buf->data;
            wvec[iocnt].iov_len = buf->size;
            iocnt++;
        }
        ssize_t bytes_transferred = writev(client->sock, wvec, iocnt);
        if (bytes_transferred == -1)
        {
            // Unable to send data?
            int e = errno;
            fprintf(stderr, "unable to write: %s\n", strerror(e));
            if (http_server_poll_client(it, HTTP_SERVER_POLL_REMOVE) != HTTP_SERVER_OK)
            {
                return HTTP_SERVER_SOCKET_ERROR;
            }
            if (srv->closesocket_func(it->sock, srv->closesocket_data) != HTTP_SERVER_OK)
            {
                return HTTP_SERVER_SOCKET_ERROR;
            }
            //perror("write");
            // int e = errno;
            // fprintf(stderr, "Unable to write data to client %d: %d\n", e, bytes_transferred);
            return HTTP_SERVER_SOCKET_ERROR;
        }
        fprintf(stderr, "Client %d: written %d bytes\n", client->sock, (int)bytes_transferred);
        // Pop buffers from response
        iocnt = 0;
        buf = NULL;
        while (!TAILQ_EMPTY(&client->buffer))
        {
            buf = TAILQ_FIRST(&client->buffer);
            if (iocnt >= maxiov)
            {
                break;
            }
            if (bytes_transferred >= buf->size)
            {
                TAILQ_REMOVE(&client->buffer, buf, bufs);
                free(buf->mem);
                bytes_transferred -= buf->size;
                free(buf);
            }
            iocnt++;
        }
        // It is possible that writev(2) will not write all data
        if (bytes_transferred > 0 && !TAILQ_EMPTY(&client->buffer))
        {
            // This is probably buggy
            fprintf(stderr, "truncate first buffer\n");
            buf = TAILQ_FIRST(&client->buffer);
            if (bytes_transferred > buf->size)
            {
                fprintf(stderr, "there is too much to truncate: %d > %d\n", (int)bytes_transferred, buf->size);
                return HTTP_SERVER_OK;
            }
            // Truncate first buffer
            assert(bytes_transferred <= buf->size);
            //memmove(buf->data, buf->data + bytes_transferred, bytes_transferred);
            buf->data += bytes_transferred;
            buf->size -= bytes_transferred;
            assert(buf->size > 0);
            bytes_transferred = 0;
        }
        // Poll again if there is any data left
        if (!TAILQ_EMPTY(&client->buffer) && http_server_poll_client(client, HTTP_SERVER_POLL_OUT) != HTTP_SERVER_OK)
        {
            return HTTP_SERVER_SOCKET_ERROR;
        }
        // If user finishes the response with `http_server_response_end` then
        // it is clear when to proceed to the next response.
        http_server_response * res = client->current_response_;
        if (res && res->is_done)
        {
            // All response data is sent now. No need to hold this memory,
            // and allow next requests inside the same connection to create
            // new responses.
            http_server_response_free(res);
            client->current_response_ = NULL;
            // All response is sent. Poll again for new request
            if (http_server_poll_client(client, HTTP_SERVER_POLL_IN) != HTTP_SERVER_OK)
            {
                fprintf(stderr, "unable to poll in for next request on client %d\n", client->sock);
                return HTTP_SERVER_SOCKET_ERROR;
            }
        }
    }
    return r;
}
