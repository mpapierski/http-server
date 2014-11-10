#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <assert.h>

static int my_url_callback(http_parser * parser, const char * at, size_t length)
{
    http_server_client * client = parser->data;
    fprintf(stderr, "url chunk: %.*s\n", (int)length, at);
    return client->handler_->on_url(client, client->handler_->on_url_data, at, length);
}

static int my_message_complete_callback(http_parser * parser)
{
    http_server_client * client = parser->data;
    fprintf(stderr, "message complete\n");
    return client->handler_->on_message_complete(client, client->handler_->on_message_complete_data);
}


static int my_on_body(http_parser * parser, const char * at, size_t length)
{
    http_server_client * client = parser->data;
    fprintf(stderr, "body: %.*s\n", (int)length, at);
    return 0;
}


http_server_client * http_server_new_client(http_server_socket_t sock, http_server_handler * handler)
{
    http_server_client * client = malloc(sizeof(http_server_client));
    client->sock = sock;
    client->data = NULL;
    client->handler_ = handler;
    // Initialize http parser stuff
    bzero(&client->parser_settings_, sizeof(client->parser_settings_));
    // Request
    http_parser_init(&client->parser_, HTTP_REQUEST);
    client->parser_.data = client;
    client->parser_settings_.on_url = &my_url_callback;
    client->parser_settings_.on_body = &my_on_body;
    client->parser_settings_.on_message_complete = &my_message_complete_callback;
    return client;
}

int http_server_perform_client(http_server_client * client, const char * at, size_t size)
{
    assert(client);
    int nparsed = http_parser_execute(&client->parser_, &client->parser_settings_, at, size);
    fprintf(stderr, "parse %d/%d\n", (int)size, nparsed);
    if (nparsed != size)
    {
        // Error
        fprintf(stderr, "unable to execute parser %d/%d\n", (int)nparsed, (int)size);
        return HTTP_SERVER_PARSER_ERROR;
    }
    return HTTP_SERVER_OK;
}
