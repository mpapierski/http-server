#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uv.h>
#include "http-server/http-server.h"

uv_loop_t * loop;
http_server srv;
http_server_handler handler;

typedef struct
{
    uv_poll_t poll_handle;
    http_server_socket_t sockfd;
} client_context;

client_context * create_client_context(http_server_socket_t fd)
{
    client_context * ctx = malloc(sizeof(client_context));
    ctx->sockfd = fd;
    uv_poll_init_socket(loop, &ctx->poll_handle, ctx->sockfd);
    ctx->poll_handle.data = ctx;
    return ctx;
}

void close_cb(uv_handle_t * handle)
{
  client_context * context = (client_context *) handle->data;
  free(context);
  fprintf(stderr, "closed poll handle\n");
}
 
void destroy_http_context(client_context * context)
{
    uv_close((uv_handle_t*) &context->poll_handle, &close_cb);
}

void http_perform(uv_poll_t *req, int status, int events)
{
    fprintf(stderr, "perform (status=%d)\n", status);
    client_context * ctx = req->data;
    int flags = 0;
    if (events & UV_READABLE)
    {
        flags |= HTTP_SERVER_POLL_IN;
    }
    if (events & UV_WRITABLE)
    {
        flags |= HTTP_SERVER_POLL_OUT;
    }
    assert(flags);
    fprintf(stderr, "execute socket action %d\n", ctx->sockfd);
    uv_poll_stop(req);
    http_server_socket_action(&srv, ctx->sockfd, flags);
}

int _socket_function(void * clientp, http_server_socket_t sock, int flags, void * socketp)
{
    client_context * ctx = socketp;
    if (!ctx)
    {
        fprintf(stderr, "create new context for %d\n", sock);
        ctx = create_client_context(sock);
        http_server_assign(&srv, sock, ctx);
    }
    assert(ctx);
    if (flags & HTTP_SERVER_POLL_IN)
    {
        uv_poll_start(&ctx->poll_handle, UV_READABLE, http_perform);
    }
    if (flags & HTTP_SERVER_POLL_OUT)
    {
        uv_poll_start(&ctx->poll_handle, UV_WRITABLE, http_perform);
    }
    if (flags & HTTP_SERVER_POLL_REMOVE)
    {
        destroy_http_context((client_context*) socketp);
    }
    return HTTP_SERVER_OK;
}

// HTTP handler callbacks

int on_message_complete(http_server_client * client, void * data)
{
    char * url;
    int r = http_server_client_getinfo(client, HTTP_SERVER_CLIENTINFO_URL, &url);
    assert(r == HTTP_SERVER_OK);
    fprintf(stderr, "Message complete %s\n", url);
    http_server_response * res = http_server_response_new();
    assert(res);
    r = http_server_response_begin(client, res);
    assert(r == HTTP_SERVER_OK);
    r = http_server_response_write_head(res, 200);
    assert(r == HTTP_SERVER_OK);
    char chunk[1024];
    int len = sprintf(chunk, "Hello world!\n");
    r = http_server_response_write(res, chunk, len);
    assert(r == HTTP_SERVER_OK);
    r = http_server_response_end(res);
    assert(r == HTTP_SERVER_OK);
    return 0;
}

int main(int argc, char * argv[])
{
    signal(SIGPIPE, SIG_IGN);
    loop = uv_default_loop();
    int result;
    http_server_init(&srv);
    srv.sock_listen_data = NULL; // should be null by default

    http_server_handler_init(&handler);
    handler.on_message_complete = &on_message_complete;

    result = http_server_setopt(&srv, HTTP_SERVER_OPT_HANDLER, &handler);
    result = http_server_setopt(&srv, HTTP_SERVER_OPT_HANDLER_DATA, &srv);

    // Set options
    //result = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_DATA, NULL);
    
    //result = http_server_setopt(&srv, HTTP_SERVER_OPT_OPEN_SOCKET_FUNCTION, &_opensocket_function);

    result = http_server_setopt(&srv, HTTP_SERVER_OPT_SOCKET_DATA, NULL);
    result = http_server_setopt(&srv, HTTP_SERVER_OPT_SOCKET_FUNCTION, &_socket_function);

    http_server_start(&srv);
    result = uv_run(loop, UV_RUN_DEFAULT);
    http_server_free(&srv);
    return result;
}
