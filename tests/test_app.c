#include "http-server/http-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>
#include <string.h>

typedef struct 
{
    char url[1024];
} http_server_request;

#define ASSERT(expr) do { if (!(expr)) { fprintf(stderr, "Error! assert(" #expr ") failed.\n"); abort(); }} while (0)

int on_url(http_server_client * client, void * data, const char * buf, size_t size)
{
    http_server_request * request = client->data;
    if (!request)
    {
        client->data = calloc(1, sizeof(http_server_request));
        request = client->data;
    }
    fprintf(stderr, "URL chunk: %.*s\n", (int)size, buf);
    strncpy(request->url, buf, size);
    return 0;
}

int on_message_complete(http_server_client * client, void * data)
{
    http_server_request * req = client->data;
    fprintf(stderr, "Message complete\n");
    http_server_response * res = http_server_response_new();
    ASSERT(res);
    int r = http_server_response_begin(client, res);
    ASSERT(r == HTTP_SERVER_OK);
    if (strncmp(req->url, "/set-headers/", sizeof(req->url) - 1) == 0)
    {
        for (int i = 0; i < 10; ++i)
        {
            char key[16], value[16];
            int key_size = snprintf(key, sizeof(key), "Key%d", i);
            int value_size = snprintf(value, sizeof(value), "Value%d", i);
            r = http_server_response_set_header(res, key, key_size, value, value_size);
            ASSERT(r == HTTP_SERVER_OK);
        }
        r = http_server_response_write_head(res, 200);
        ASSERT(r == HTTP_SERVER_OK);
        r = http_server_response_printf(res, "url=%s\n", req->url);
        ASSERT(r == HTTP_SERVER_OK);
    }
    else if (strncmp(req->url, "/get/", sizeof(req->url) - 1) == 0)
    {
        if (client->parser_.method == HTTP_GET)
        {
            r = http_server_response_write_head(res, 200);
            ASSERT(r == HTTP_SERVER_OK);
            r = http_server_response_printf(res, "url=%s\n", req->url);
            ASSERT(r == HTTP_SERVER_OK);
        }
        else
        {
            r = http_server_response_write_head(res, 405);
            ASSERT(r == HTTP_SERVER_OK);
        }
    }
    else if (strncmp(req->url, "/cancel/", sizeof(req->url) - 1) == 0)
    {
        int result = http_server_cancel(client->server_);
        r = http_server_response_write_head(res, 200);
        ASSERT(r == HTTP_SERVER_OK);
        r = http_server_response_printf(res, "success=%d\n", result);
        ASSERT(r == HTTP_SERVER_OK);
    }
    else
    {
        r = http_server_response_write_head(res, 404);
        ASSERT(r == HTTP_SERVER_OK);
    }
    r = http_server_response_end(res);
    ASSERT(r == HTTP_SERVER_OK);
    free(req);
    client->data = NULL;
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
    bzero(&handler, sizeof(handler));
    handler.on_url = &on_url;
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
