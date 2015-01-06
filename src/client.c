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
        http_server_header_free(header);
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
        if (http_server_string_append(&client->header_field_, at, length) != HTTP_SERVER_OK)
        {
            return 1;
        }
    }
    else if (client->header_state_ == 'V')
    {
        // We have field and value.
        struct http_server_header * new_header = http_server_header_new();
        if (!new_header)
        {
            return 1;
        }
        // Move temporary fields into new header structure
        http_server_string_move(&client->header_field_, &new_header->field);
        http_server_string_move(&client->header_value_, &new_header->value);
        //fprintf(stderr, "new header key[%s] value[%s]\n", http_server_string_str(&new_header->field), http_server_string_str(&new_header->value));
        TAILQ_INSERT_TAIL(&client->headers, new_header, headers);
        if (client->handler && client->handler->on_header)
        {
            result = client->handler->on_header(client, client->handler->on_header_data, http_server_string_str(&new_header->field), http_server_string_str(&new_header->value));
        }
        
        if (http_server_string_append(&client->header_field_, at, length) != HTTP_SERVER_OK)
        {
            return 1;
        }
    }
    else if (client->header_state_ == 'F')
    {
        if (http_server_string_append(&client->header_field_, at, length) != HTTP_SERVER_OK)
        {
            return 1;
        }
    }
    client->header_state_ = 'F';
    return result;
}

static int my_on_header_value(http_parser * parser, const char * at, size_t length)
{
    http_server_client * client = parser->data;
    if (client->header_state_ == 'F')
    {
        if (http_server_string_append(&client->header_value_, at, length) != HTTP_SERVER_OK)
        {
            return 1;
        }
    }
    else if (client->header_state_ == 'V')
    {
        if (http_server_string_append(&client->header_value_, at, length) != HTTP_SERVER_OK)
        {
            return 1;
        }
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

    struct http_server_header * new_header = http_server_header_new();
    if (!new_header)
    {
        return HTTP_SERVER_NO_MEMORY;
    }
    // Move temporary fields into new header structure
    http_server_string_move(&client->header_field_, &new_header->field);
    http_server_string_move(&client->header_value_, &new_header->value);
    // Clean up memory, because this client instance might be reused.
    client->header_state_ = 'S';
    TAILQ_INSERT_TAIL(&client->headers, new_header, headers);
    int r = 0;
    if (client->handler && client->handler->on_header)
    {
        r = client->handler->on_header(client, client->handler->on_header_data, http_server_string_str(&new_header->field), http_server_string_str(&new_header->value));
    }
    // Look for "Expect: 100-continue"
    struct http_server_header * header;
    TAILQ_FOREACH(header, &client->headers, headers)
    {
        fprintf(stderr, "header key[%s] value[%s]\n", http_server_string_str(&header->field), http_server_string_str(&header->value));
        if (strcasecmp(http_server_string_str(&header->field), "Expect") == 0
            && strcasecmp(http_server_string_str(&header->value), "100-continue") == 0)
        {
            // TODO: Execute user specified callback that handles 100-continue
            char * status_line = "HTTP/1.1 100 Continue\r\n\r\n";
            fprintf(stderr, "send 100 continue\n");
            if (http_server_client_write(client, status_line, strlen(status_line)) != HTTP_SERVER_OK)
            {
                fprintf(stderr, "unable to write data to client %d\n", client->sock);
                return 1;
            }
            if (http_server_client_flush(client) != HTTP_SERVER_OK)
            {
                fprintf(stderr, "unable to flush data to client %d\n", client->sock);
                return 1;
            }
            break;
        }
    }
    return r;
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
    // Create new queue of outgoing chunks of data
    TAILQ_INIT(&client->buffer);
    // Initialize string which will hold full URL data
    http_server_string_init(&client->url);
    // Initialize request headers
    TAILQ_INIT(&client->headers);
    // Temporary data for incomming request headers
    client->header_state_ = 'S';
    http_server_string_init(&client->header_field_);
    http_server_string_init(&client->header_value_);
    client->is_paused_ = 0;
    return client;
}

void http_server_client_free(http_server_client * client)
{
    http_server__client_free_headers(client);
    http_server_string_free(&client->header_field_);
    http_server_string_free(&client->header_value_);
    // Free queued buffers
    while (!TAILQ_EMPTY(&client->buffer))
    {
        http_server_buf * buf = TAILQ_FIRST(&client->buffer);
        assert(buf);
        TAILQ_REMOVE(&client->buffer, buf, bufs);
        free(buf->mem);
        free(buf);
    }
    // Free URL data
    http_server_string_free(&client->url);
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
    //assert(client->current_flags & HTTP_SERVER_POLL_OUT )
    if (client->current_flags & HTTP_SERVER_POLL_IN)
    {
        assert(!client->is_paused_);
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

int http_server_client_write(http_server_client * client, char * data, int size)
{
    if (!client)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    http_server_buf * new_buffer = malloc(sizeof(http_server_buf));
    if (!new_buffer)
    {
        return HTTP_SERVER_NO_MEMORY;
    }
    new_buffer->mem = new_buffer->data = malloc(size + 1);
    if (!new_buffer->data)
    {
        free(new_buffer);
        return HTTP_SERVER_NO_MEMORY;
    }
    memcpy(new_buffer->data, data, size);
    new_buffer->data[size] = '\0';
    new_buffer->size = size;
    TAILQ_INSERT_TAIL(&client->buffer, new_buffer, bufs);
    return HTTP_SERVER_OK;
}

int http_server_client_flush(http_server_client * client)
{
    if (TAILQ_EMPTY(&client->buffer))
    {
        return HTTP_SERVER_OK;
    }
    return http_server_poll_client(client, HTTP_SERVER_POLL_OUT);
}

int http_server_client_pause(http_server_client * client, int pause)
{
    if (!client)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    int previous_state = client->is_paused_;
    fprintf(stderr, "change paused on %d from %d->%d\n", client->sock, previous_state, pause);
    client->is_paused_ = pause;
    // If paused state is enabled after it was disabled then we try
    // to read data again.
    if (previous_state == 1 && pause == 0)
    {
        return http_server_poll_client(client, HTTP_SERVER_POLL_IN);
    }
    return HTTP_SERVER_OK;
}
