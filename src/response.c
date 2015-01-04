#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

static int http_server_header_cmp(struct http_server_header * lhs, struct http_server_header * rhs)
{
    const char * a = http_server_string_str(&lhs->field);
    const char * b = http_server_string_str(&rhs->field);
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
    new_buffer->mem = new_buffer->data = malloc(size + 1);
    if (!new_buffer->data)
    {
        free(new_buffer);
        return -1;
    }
    memcpy(new_buffer->data, data, size);
    new_buffer->data[size] = '\0';
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
    res->is_done = 0;
    return res;
}

void http_server_response_free(http_server_response * res)
{
    if (res == 0)
    {
        return;
    }
    // Free queued buffers
    while (!TAILQ_EMPTY(&res->buffer))
    {
        http_server_buf * buf = TAILQ_FIRST(&res->buffer);
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
        http_server_string_free(&header->field);
        http_server_string_free(&header->value);
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
    if (http_server_poll_client(client, HTTP_SERVER_POLL_OUT) != HTTP_SERVER_OK)
    {
        return HTTP_SERVER_SOCKET_ERROR;
    }
    return HTTP_SERVER_OK;
}

int http_server_response_end(http_server_response * res)
{
    assert(res);
    assert(res->is_done == 0);
    // Mark the response as "finished" so we can know when to proceed
    // to the next request.
    res->is_done = 1;
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
    struct http_server_header * hdr = http_server_header_new();
    int r;
    if ((r = http_server_string_append(&hdr->field, name, namelen)) != HTTP_SERVER_OK)
    {
        return r;
    }
    if ((r = http_server_string_append(&hdr->value, value, valuelen)) != HTTP_SERVER_OK)
    {
        return r;
    }
    
    // Disable chunked encoding if user specifies length
    struct http_server_header hdr_contentlength;
    http_server_string_init(&hdr_contentlength.field);
    if ((r = http_server_string_append(&hdr_contentlength.field, "Content-Length", 14)) != HTTP_SERVER_OK)
    {
        return r;
    }
    struct http_server_header hdr_transferencoding;
    http_server_string_init(&hdr_transferencoding.field);
    if ((r = http_server_string_append(&hdr_transferencoding.field, "Transfer-Encoding", 17)) != HTTP_SERVER_OK)
    {
        return r;
    }
    
    // Check if user tries to set Content-length header, so we
    // have to disable chunked encoding.
    if (http_server_header_cmp(&hdr_contentlength, hdr) == 0)
    {
        // Remove Transfer-encoding if user sets content-length

        while (!TAILQ_EMPTY(&res->headers))
        {
            struct http_server_header * header = TAILQ_FIRST(&res->headers);
            if (http_server_header_cmp(&hdr_transferencoding, header) == 0)
            {
                TAILQ_REMOVE(&res->headers, header, headers);
                http_server_header_free(header);
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
        while (!TAILQ_EMPTY(&res->headers))
        {
            // Add all queued headers to the response queue
            struct http_server_header * header = TAILQ_FIRST(&res->headers);
            char data[1024];
            int data_len = sprintf(data, "%s: %s\r\n", http_server_string_str(&header->field), http_server_string_str(&header->value));
            int r = _response_add_buffer(res, data, data_len);
            // Remove first header from the queue
            TAILQ_REMOVE(&res->headers, header, headers);
            http_server_header_free(header);
            if (r != HTTP_SERVER_OK)
            {
                return r;
            }
        }
        int r = _response_add_buffer(res, "\r\n", 2);
        if (r != HTTP_SERVER_OK)
        {
            return r;
        }
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
        if (!data || size == 0)
        {
            // Do nothing - called once will create empty response.
            return http_server_response__flush(res);
        }
        int r = _response_add_buffer(res, data, size);
        assert(r == 0);
    }
    return http_server_response__flush(res);
}

int http_server_response_printf(http_server_response * res, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    char * buffer = NULL;
    int result = vasprintf(&buffer, format, args);
    assert(buffer);
    int r = http_server_response_write(res, buffer, result);
    free(buffer);
    va_end(args);
    return r;
}
