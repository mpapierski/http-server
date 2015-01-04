#include "http-server/http-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>
#include <string.h>

typedef struct 
{
    char body[1024];
    int headers_received;
} http_server_request;

http_server_request * create_request(void)
{
    http_server_request * req = malloc(sizeof(http_server_request));
    memset(req->body, '\0', sizeof(req->body));
    req->headers_received = 0;
    return req;
}

#define ASSERT(expr) do { if (!(expr)) { fprintf(stderr, "Error! assert(" #expr ") failed.\n"); abort(); }} while (0)

int on_body(http_server_client * client, void * data, const char * buf, size_t size)
{
    http_server_request * request = client->data;
    assert(request);
    strncpy(request->body, buf, size);
    return 0;
}

int on_header(http_server_client * client, void * data, const char * field, const char * value)
{
    http_server_request * request = client->data;
    if (!request)
    {
        client->data = create_request();
        request = client->data;
    }
    request->headers_received += 1;
    return 0;
}

int on_message_complete(http_server_client * client, void * data)
{
    http_server_request * req = client->data;
    fprintf(stderr, "Message complete\n");
    http_server_response * res = http_server_response_new();
    ASSERT(res);
    char * url;
    int r = http_server_client_getinfo(client, HTTP_SERVER_CLIENTINFO_URL, &url);
    ASSERT(r == HTTP_SERVER_OK);
    r = http_server_response_begin(client, res);
    ASSERT(r == HTTP_SERVER_OK);
    assert(url);
    if (strncmp(url, "/set-headers/", 13) == 0)
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
        r = http_server_response_printf(res, "url=%s\n", url);
        ASSERT(r == HTTP_SERVER_OK);
    }
    else if (strncmp(url, "/get/", 5) == 0)
    {
        if (client->parser_.method == HTTP_GET)
        {
            r = http_server_response_write_head(res, 200);
            ASSERT(r == HTTP_SERVER_OK);
            r = http_server_response_printf(res, "url=%s\n", url);
            ASSERT(r == HTTP_SERVER_OK);
            struct http_server_header * header;
            TAILQ_FOREACH(header, &client->headers, headers)
            {
                r = http_server_response_printf(res, "%s=%s\n", header->key, header->value);
                ASSERT(r == HTTP_SERVER_OK);
            }
            r = http_server_response_printf(res, "total_headers=%d\n", req->headers_received);
            ASSERT(r == HTTP_SERVER_OK);
        }
        else
        {
            r = http_server_response_write_head(res, 405);
            ASSERT(r == HTTP_SERVER_OK);
        }
    }
    else if (strncmp(url, "/post/", 6) == 0)
    {
        if (client->parser_.method == HTTP_POST)
        {
            r = http_server_response_write_head(res, 200);
            ASSERT(r == HTTP_SERVER_OK);
            r = http_server_response_printf(res, "body=%s\n", req->body);
            ASSERT(r == HTTP_SERVER_OK);
        }
        else
        {
            r = http_server_response_write_head(res, 405);
            ASSERT(r == HTTP_SERVER_OK);
        }
    }
    else if (strncmp(url, "/cancel/", 8) == 0)
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
    result = http_server_handler_init(&handler);
    ASSERT(result == HTTP_SERVER_OK);
    handler.on_message_complete = &on_message_complete;
    handler.on_body = &on_body;
    handler.on_header = &on_header;
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
