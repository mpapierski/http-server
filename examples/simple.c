#include "http-server/http-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

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
    int exit_code;
    http_server srv;
    // Inits data structure
    int result;
    if ((result = http_server_init(&srv)) != HTTP_SERVER_OK)
    {
        fprintf(stderr, "Unable to init http server instance: %s\n", http_server_errstr(result));
        return 1;
    }
    // Init handler function
    http_server_handler handler;
    http_server_handler_init(&handler);
    handler.on_message_complete = &on_message_complete;
    if ((result = http_server_setopt(&srv, HTTP_SERVER_OPT_HANDLER, &handler)) != HTTP_SERVER_OK)
    {
        fprintf(stderr, "Unable to set handler: %s\n", http_server_errstr(result));
        return 1;
    }
    if ((result = http_server_setopt(&srv, HTTP_SERVER_OPT_HANDLER_DATA, &srv)) != HTTP_SERVER_OK)
    {
        fprintf(stderr, "Unable to set handler data: %s\n", http_server_errstr(result));
        return 1;
    }

    // Initializes stuff
    if ((result = http_server_start(&srv)) != HTTP_SERVER_OK)
    {
        fprintf(stderr, "Unable to start http server: %s\n", http_server_errstr(result));
        return 1;
    }
    exit_code = http_server_run(&srv);
    // Cleans up everything
    http_server_free(&srv);
    return exit_code;
}
