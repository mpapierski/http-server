#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <assert.h>

static void http_server__client_free_headers(http_server_client * client)
{
    while (!TAILQ_EMPTY(&client->headers))
    {
        struct http_server_header * header = TAILQ_FIRST(&client->headers);
        TAILQ_REMOVE(&client->headers, header, headers);
        free(header->key);
        free(header->value);
        free(header);
    }
}

static int my_url_callback(http_parser * parser, const char * at, size_t length)
{
    http_server_client * client = parser->data;
    fprintf(stderr, "url chunk: %.*s\n", (int)length, at);
    int rv = 0;
    if (http_server_string_append(&client->url, at, length) != HTTP_SERVER_OK)
    {
        // Failed to add more memory to the URL. Failure, stop.
        fprintf(stderr, "failed to add more moemory to url\n");
        return 1;
    }
    return rv;
}

static int my_message_complete_callback(http_parser * parser)
{
    http_server_client * client = parser->data;
    fprintf(stderr, "message complete\n");
    int rv = 0;
    if (client->handler && client->handler->on_message_complete)
    {
        rv = client->handler->on_message_complete(client, client->handler->on_message_complete_data);
    }
    client->is_complete = 1;
    http_server__client_free_headers(client);
    http_server_string_clear(&client->url);
    return rv;
}

static int my_on_body(http_parser * parser, const char * at, size_t length)
{
    http_server_client * client = parser->data;
    fprintf(stderr, "body: %.*s\n", (int)length, at);
    int rv;
    if (client->handler && client->handler->on_body)
    {
        rv = client->handler->on_body(client, client->handler->on_body_data, at, length);
    }
    return rv;
}

static int my_on_header_field(http_parser * parser, const char * at, size_t length)
{
    http_server_client * client = parser->data;
    int result = 0;
    if (client->header_state_ == 'S')
    {
        if (client->header_field_len_ + length >= client->header_field_size_)
        {
            char * new_field = realloc(client->header_field_, sizeof(char) * (client->header_field_size_ + length + 1));
            if (!new_field)
            {
                abort();
            }
            client->header_field_ = new_field;
            client->header_field_size_ += length;
        }
        int current_len = client->header_field_len_;
        memcpy(client->header_field_ + current_len, at, length);
        client->header_field_[current_len + length] = '\0';
        client->header_field_len_ += length;
    }
    else if (client->header_state_ == 'V')
    {
        // We have field and value.
        struct http_server_header * new_header = malloc(sizeof(struct http_server_header));
        if (!new_header)
        {
            abort();
        }
        // Move temporary fields into new header structure
        new_header->key = client->header_field_;
        new_header->key_size = client->header_field_size_;
        new_header->value = client->header_value_;
        new_header->value_size = client->header_value_size_;
        fprintf(stderr, "new header key[%.*s] value[%.*s]\n", new_header->key_size, new_header->key, new_header->value_size, new_header->value);
        TAILQ_INSERT_TAIL(&client->headers, new_header, headers);
        if (client->handler && client->handler->on_header)
        {
            result = client->handler->on_header(client, client->handler->on_header_data, new_header->key, new_header->value);
        }
        // Clear out pointers to allow new fields to be populated 
        client->header_field_ = NULL;
        client->header_field_size_ = 0;
        client->header_field_len_ = 0;
        client->header_value_ = NULL;
        client->header_value_size_ = 0;
        client->header_value_len_ = 0;

        if (client->header_field_len_ + length >= client->header_field_size_)
        {
            char * new_field = realloc(client->header_field_, sizeof(char) * (client->header_field_size_ + length + 1));
            if (!new_field)
            {
                abort();
            }
            client->header_field_ = new_field;
            client->header_field_size_ += length;
        }
        int current_len = client->header_field_len_;
        memcpy(client->header_field_ + current_len, at, length);
        client->header_field_len_ += length;
        client->header_field_[current_len + length] = '\0';
    }
    else if (client->header_state_ == 'F')
    {
        if (client->header_field_len_ + length >= client->header_field_size_)
        {
            char * new_field = realloc(client->header_field_, sizeof(char) * (client->header_field_size_ + length + 1));
            if (!new_field)
            {
                abort();
            }
            client->header_field_ = new_field;
            client->header_field_size_ += length;
        }
        int current_len = client->header_field_len_;
        memcpy(client->header_field_ + current_len, at, length);
        client->header_field_[current_len + length] = '\0';
        client->header_field_len_ += length;
    }
    client->header_state_ = 'F';
    return result;
}

static int my_on_header_value(http_parser * parser, const char * at, size_t length)
{
    http_server_client * client = parser->data;
    if (client->header_state_ == 'F')
    {
        if (client->header_value_len_ + length >= client->header_value_size_)
        {
            char * new_field = realloc(client->header_value_, sizeof(char) * (client->header_value_size_ + length + 1));
            if (!new_field)
            {
                abort();
            }
            client->header_value_ = new_field;
            client->header_value_size_ += length;
        }
        int current_len = client->header_value_len_;
        memcpy(client->header_value_ + current_len, at, length);
        client->header_value_[current_len + length] = '\0';
        client->header_value_len_ += length;
    }
    else if (client->header_state_ == 'V')
    {
        if (client->header_value_len_ + length >= client->header_value_size_)
        {
            char * new_field = realloc(client->header_value_, sizeof(char) * (client->header_value_size_ + length + 1));
            if (!new_field)
            {
                abort();
            }
            client->header_value_ = new_field;
            client->header_value_size_ += length;
        }
        int current_len = client->header_value_len_;
        memcpy(client->header_value_ + current_len, at, length);
        client->header_value_[current_len + length] = '\0';
        client->header_value_len_ += length;
    }
    client->header_state_ = 'V';
    return 0;
}

int my_on_headers_complete(http_parser * parser)
{
    http_server_client * client = parser->data;
    if (client->header_state_ != 'V')
    {
        return 0;
    }

    struct http_server_header * new_header = malloc(sizeof(struct http_server_header));
    if (!new_header)
    {
        abort();
    }
    // Move temporary fields into new header structure
    new_header->key = client->header_field_;
    new_header->key_size = client->header_field_size_;
    new_header->value = client->header_value_;
    new_header->value_size = client->header_value_size_;
    // Clean up memory, because this client instance might be reused.
    client->header_field_ = NULL;
    client->header_field_size_ = 0;
    client->header_field_len_ = 0;
    client->header_value_ = NULL;
    client->header_value_size_ = 0;
    client->header_value_len_ = 0;
    client->header_state_ = 'S';
    fprintf(stderr, "new header key[%.*s] value[%.*s]\n", new_header->key_size, new_header->key, new_header->value_size, new_header->value);
    TAILQ_INSERT_TAIL(&client->headers, new_header, headers);
    if (client->handler && client->handler->on_header)
    {
        return client->handler->on_header(client, client->handler->on_header_data, new_header->key, new_header->value);
    }
    return 0;
}

http_server_client * http_server_new_client(http_server * server, http_server_socket_t sock, http_server_handler * handler)
{
    http_server_client * client = malloc(sizeof(http_server_client));
    client->sock = sock;
    client->data = NULL;
    client->handler = handler;
    // Initialize http parser stuff
    bzero(&client->parser_settings_, sizeof(client->parser_settings_));
    // Request
    http_parser_init(&client->parser_, HTTP_REQUEST);
    client->parser_.data = client;
    client->parser_settings_.on_url = &my_url_callback;
    client->parser_settings_.on_body = &my_on_body;
    client->parser_settings_.on_message_complete = &my_message_complete_callback;
    client->parser_settings_.on_header_field = &my_on_header_field;
    client->parser_settings_.on_header_value = &my_on_header_value;
    client->parser_settings_.on_headers_complete = &my_on_headers_complete;
    // Response should be NULL so we know when to poll for WRITE
    client->current_response_ = NULL;
    client->server_ = server;
    client->current_flags = 0;
    client->is_complete = 0;
    // Initialize string which will hold full URL data
    http_server_string_init(&client->url);
    // Initialize request headers
    TAILQ_INIT(&client->headers);
    // Temporary data for incomming request headers
    client->header_state_ = 'S';
    client->header_field_ = NULL;
    client->header_field_len_ = 0;
    client->header_field_size_ = 0;
    client->header_value_ = NULL;
    client->header_value_len_ = 0;
    client->header_value_size_ = 0;
    return client;
}

void http_server_client_free(http_server_client * client)
{
    http_server__client_free_headers(client);
    free(client);
}

int http_server_perform_client(http_server_client * client, const char * at, size_t size)
{
    assert(client);
    int nparsed = http_parser_execute(&client->parser_, &client->parser_settings_, at, size);
    fprintf(stderr, "parse %d/%d\n", (int)size, nparsed);
    if (nparsed != size)
    {
        const char * err = http_errno_description(client->parser_.http_errno);
        // Error
        fprintf(stderr, "unable to execute parser %d/%d (%s)\n", (int)nparsed, (int)size, err);
        return HTTP_SERVER_PARSER_ERROR;
    }
    return HTTP_SERVER_OK;
}

int http_server_poll_client(http_server_client * client, int flags)
{
    if (!client)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    if (!client->server_)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    if (!client->server_->socket_func)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    int old_flags = client->current_flags;
    client->current_flags |= flags;
    if (old_flags == client->current_flags)
    {
        // To save calls to user provided callbacks ignore
        // a case when I/O poll flags didnt changed.
        return HTTP_SERVER_OK;
    }
    if (client->server_->socket_func(client->server_->socket_data, client->sock, client->current_flags, client->data) != HTTP_SERVER_OK)
    {
        return HTTP_SERVER_SOCKET_ERROR;
    }
    return HTTP_SERVER_OK;
}

int http_server_client_getinfo(http_server_client * client, http_server_clientinfo code, ...)
{
    if (!client)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    if (code == HTTP_SERVER_CLIENTINFO_URL)
    {
        va_list ap;
        va_start(ap, code);
        char ** url = va_arg(ap, char **);
        *url = (char *)http_server_string_str(&client->url);
        va_end(ap);
    }
    else
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    return HTTP_SERVER_OK;
}
