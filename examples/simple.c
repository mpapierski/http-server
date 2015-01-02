#include "http-server/http-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

int on_url(http_server_client * client, void * data, const char * buf, size_t size)
{
    fprintf(stderr, "URL chunk: %.*s\n", (int)size, buf);
    return 0;
}

int on_message_complete(http_server_client * client, void * data)
{
    fprintf(stderr, "Message complete\n");
    http_server_response * res = http_server_response_new();
    assert(res);
    int r = http_server_response_begin(client, res);
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
    int exit_code;
    http_server srv;
    // Inits data structure
    http_server_init(&srv);
    // Init handler function
    http_server_handler handler;
    bzero(&handler, sizeof(handler));
    handler.on_url = &on_url;
    handler.on_message_complete = &on_message_complete;
    http_server_setopt(&srv, HTTP_SERVER_OPT_HANDLER, &handler);
    http_server_setopt(&srv, HTTP_SERVER_OPT_HANDLER_DATA, &srv);
    // Initializes stuff
    http_server_start(&srv);
    exit_code = http_server_run(&srv);
    // Cleans up everything
    http_server_free(&srv);
    return exit_code;
}
