#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

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
    return res;
}

void http_server_response_free(http_server_response * res)
{
    assert(res);
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
    if (res->client->server_->socket_func(res->client->server_->socket_data, res->client->sock, HTTP_SERVER_POLL_OUT, res->client->data) != HTTP_SERVER_OK)
    {
        return HTTP_SERVER_SOCKET_ERROR;
    }
    return HTTP_SERVER_OK;
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
    char header[1024];
    int header_len = sprintf(header, "%.*s: %.*s\r\n", namelen, name, valuelen, value);
    int r = _response_add_buffer(res, header, header_len);
    assert(r == 0);
    return HTTP_SERVER_OK;
}

int http_server_response_write(http_server_response * res, char * data, int size)
{
    // TODO: Check for headers, do content encoding
    char * frame = malloc(1024 + size);
    int frame_length = sprintf(frame, "%x\r\n%.*s\r\n", size, size, data);
    int r = _response_add_buffer(res, frame, frame_length);
    assert(r == 0);
    free(frame);
    return http_server_response__flush(res);
}
