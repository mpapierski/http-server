#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
static int http_server_header_cmp(struct http_server_header * lhs, struct http_server_header * rhs)
{
    char * a = lhs->key;
    char * b = rhs->key;
    while( *a && *b )
    {
        int r = tolower(*a) - tolower(*b);
        if( r )
            return r;
        ++a;
        ++b;
    }
    return tolower(*a)-tolower(*b);
}

static int _response_add_buffer(http_server_response * res, char * data, int size)
{
    http_server_buf * new_buffer = malloc(sizeof(http_server_buf));
    if (!new_buffer)
    {
        return -1;
    }
    new_buffer->mem = new_buffer->data = malloc(size);
    if (!new_buffer->data)
    {
        free(new_buffer);
        return -1;
    }
    memcpy(new_buffer->data, data, size);
    new_buffer->size = size;
    TAILQ_INSERT_TAIL(&res->buffer, new_buffer, bufs);
    return 0;
}

http_server_response * http_server_response_new()
{
    http_server_response * res = malloc(sizeof(http_server_response)); 
    if (!res)
    {
        return 0;
    }
    TAILQ_INIT(&res->buffer);
    // Initialize output headers
    res->headers_sent = 0;
    TAILQ_INIT(&res->headers);
    // By default all responses are "chunked"
    int r = http_server_response_set_header(res, "Transfer-Encoding", 17, "chunked", 7);
    if (r != HTTP_SERVER_OK)
    {
        return 0;
    }
    res->client = NULL;
    return res;
}

void http_server_response_free(http_server_response * res)
{
    if (res == 0)
    {
        return;
    }
    // Free queued buffers
    http_server_buf * buf;
    TAILQ_FOREACH(buf, &res->buffer, bufs)
    {
        assert(buf);
        TAILQ_REMOVE(&res->buffer, buf, bufs);
        free(buf->mem);
        free(buf);
    }
    // Free headers
    struct http_server_header * header;
    TAILQ_FOREACH(header, &res->headers, headers)
    {
        TAILQ_REMOVE(&res->headers, header, headers);
        free(header->key);
        free(header->value);
        free(header);
    }
    free(res);
}

int http_server_response_begin(http_server_client * client, http_server_response * res)
{
    assert(client);
    assert(!client->current_response_);
    res->client = client;
    client->current_response_ = res;
    if (client->server_->socket_func(client->server_->socket_data, client->sock, HTTP_SERVER_POLL_OUT, client->data) != HTTP_SERVER_OK)
    {
        return HTTP_SERVER_SOCKET_ERROR;
    }
    return HTTP_SERVER_OK;
}

int http_server_response_end(http_server_response * res)
{
    assert(res);
    // Pop current response and proceed to the next?
    // Add "empty frame" if there is chunked encoding
    return http_server_response_write(res, NULL, 0);
}

int http_server_response__flush(http_server_response * res)
{
    if (TAILQ_EMPTY(&res->buffer))
    {
        return HTTP_SERVER_OK;
    }
    return http_server_poll_client(res->client, HTTP_SERVER_POLL_OUT);
}

int http_server_response_write_head(http_server_response * res, int status_code)
{
    char head[1024];
    int head_len = -1;
#define XX(code, name, description) \
    case code: \
        head_len = sprintf(head, "HTTP/1.1 %d %s\r\n", code, description); \
        break;
    switch (status_code)
    {
    HTTP_SERVER_ENUM_STATUS_CODES(XX)
    default:
        assert(!"Unknown status code");
        break;
    }
    assert(head_len != -1);
    int r = _response_add_buffer(res, head, head_len);
    assert(r != -1);
    return HTTP_SERVER_OK;
#undef XX
}

int http_server_response_set_header(http_server_response * res, char * name, int namelen, char * value, int valuelen)
{
    assert(res);
    // TODO: Add support to 'trailing' headers with chunked encoding
    assert(!res->headers_sent && "Headers already sent");

    // Add new header
    struct http_server_header * hdr = malloc(sizeof(struct http_server_header));

    hdr->key = malloc(namelen + 1);
    memcpy(hdr->key, name, namelen);
    hdr->key[namelen] = '\0';
    hdr->key_size = namelen;

    hdr->value = malloc(valuelen + 1);
    memcpy(hdr->value, value, valuelen);
    hdr->value[valuelen] = '\0';
    hdr->value_size = valuelen;
    
    // Disable chunked encoding if user specifies length
    struct http_server_header hdr_contentlength;
    hdr_contentlength.key = "Content-Length";
    hdr_contentlength.key_size = 14;

    struct http_server_header hdr_transferencoding;
    hdr_transferencoding.key = "Transfer-Encoding";
    hdr_transferencoding.key_size = 17;

    if (http_server_header_cmp(&hdr_contentlength, hdr) == 0)
    {
        // Remove Transfer-encoding if user sets content-length
        struct http_server_header * header;
        TAILQ_FOREACH(header, &res->headers, headers)
        {
            if (http_server_header_cmp(&hdr_transferencoding, header) == 0)
            {
                TAILQ_REMOVE(&res->headers, header, headers);
                free(header->key);
                free(header->value);
                free(header);
            }
        }
        res->is_chunked = 0;
    }

    // If user choose chunked encoding we "cache" it
    if (http_server_header_cmp(&hdr_transferencoding, hdr) == 0)
    {
        res->is_chunked = 1;
    }

    TAILQ_INSERT_TAIL(&res->headers, hdr, headers);
    return HTTP_SERVER_OK;
}

int http_server_response_write(http_server_response * res, char * data, int size)
{
    // Flush all headers if they arent already sent
    if (!res->headers_sent)
    {
        struct http_server_header * header = NULL;
        TAILQ_FOREACH(header, &res->headers, headers)
        {
            // XXX: Make one big buffer from headers
            char data[1024];
            int data_len = sprintf(data, "%.*s: %.*s\r\n", header->key_size, header->key, header->value_size, header->value);
            int r = _response_add_buffer(res, data, data_len);

            TAILQ_REMOVE(&res->headers, header, headers);
            free(header->key);
            free(header->value);
            free(header);

            assert(r == 0);
        }
        int r = _response_add_buffer(res, "\r\n", 2);
        assert(r == 0);
        res->headers_sent = 1;
    }
    if (res->is_chunked)
    {
        // Create chunked encoding frame
        char * frame = malloc(1024 + size);
        int frame_length = sprintf(frame, "%x\r\n%.*s\r\n", size, size, data);
        int r = _response_add_buffer(res, frame, frame_length);
        assert(r == 0);
        free(frame);
    }
    else
    {
        // User most probably set 'content-length' so the data is 'raw'
        int r = _response_add_buffer(res, data, size);
        assert(r == 0);
    }
    return http_server_response__flush(res);
}
