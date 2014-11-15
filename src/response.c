#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static char * _add_frame(char * buf, int * buf_size, char * frame_data, int frame_size)
{
    // data = data + frame
    char frame[1024];
    int len = sprintf(frame, "%x\r\n%.*s\r\n", frame_size, frame_size, frame_data);
    char * new_buf = realloc(buf, *buf_size + len);
    if (!new_buf)
    {
        return NULL;
    }
    memcpy(new_buf + *buf_size, frame, len);
    *buf_size += len;
    return new_buf;
}

http_server_response * http_server_response_new()
{
    http_server_response * res = malloc(sizeof(http_server_response));
    if (!res)
    {
        return 0;
    }
    res->data_ = malloc(1024);
    if (!res->data_)
    {
        free(res);
        return 0;
    }
    bzero(res->data_, 1024);
    strcat(res->data_, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    res->size_ = strlen(res->data_);
    return res;
}

void http_server_response_free(http_server_response * res)
{
    assert(res);
    assert(res->data_);
    free(res->data_);
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
    return http_server_response_send(res, NULL, 0);
}

int http_server_response__flush(http_server_response * res)
{
    assert(res->data_ && "Unable to allocate memory"); // TODO: return ENOMEM
    assert(res->client);
    assert(res->client->server_);
    if (res->client->server_->socket_func(res->client->server_->socket_data, res->client->sock, HTTP_SERVER_POLL_OUT, res->client->data) != HTTP_SERVER_OK)
    {
        return HTTP_SERVER_SOCKET_ERROR;
    }
    return HTTP_SERVER_OK;
}

int http_server_response_send(http_server_response * res, char * data, int size)
{
    // Check for headers, do content encoding
    res->data_ = _add_frame(res->data_, &res->size_, data, size);
    return http_server_response__flush(res);
}
