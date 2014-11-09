#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

static int my_url_callback(http_parser * parser, const char * at, size_t length)
{
    fprintf(stderr, "url chunk: %.*s\n", (int)length, at);
    return 0;
}

http_server_client * http_server_new_client(http_server_socket_t sock)
{
    http_server_client * client = malloc(sizeof(http_server_client));
    client->sock = sock;
    client->data = NULL;
    // Initialize http parser stuff
    http_parser_settings settings;
    bzero(&settings, sizeof(settings));
    settings.on_url = my_url_callback;
    // Request
    http_parser_init(&client->parser_, HTTP_REQUEST);
    client->parser_.data = client;
    client->parser_settings_.on_url = &my_url_callback;
    return client;
}
