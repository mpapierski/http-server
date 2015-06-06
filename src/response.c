#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include "miniz.c"

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

http_server_response * http_server_response_new(struct http_server_client * client)
{
    http_server_response * res = malloc(sizeof(http_server_response)); 
    if (!res)
    {
        return 0;
    }
    // Initialize output headers
    res->headers_sent = 0;
    TAILQ_INIT(&res->headers);
    // By default all responses are "chunked"
    int r = http_server_response_set_header(res, "Transfer-Encoding", 17, "chunked", 7);
    if (r != HTTP_SERVER_OK)
    {
        return NULL;
    }
    res->client = NULL;
    res->is_done = 0;
    res->encoding_type = HTTP_SERVER_ENCODING_NONE;
    res->enc.zlib_stream = NULL;
    // Prepare header for case insensitive comparision
    struct http_server_header hdr_accept_encoding;
    http_server_string_init(&hdr_accept_encoding.field);
    if ((r = http_server_string_append(&hdr_accept_encoding.field, "Accept-Encoding", 15)) != HTTP_SERVER_OK)
    {
        return NULL;
    }
    // Check for Accept-Encoding header
    struct http_server_header * hdr;
    char * content_encoding = NULL;
    TAILQ_FOREACH(hdr, &client->headers, headers)
    {
        if (http_server_header_cmp(&hdr_accept_encoding, hdr) == 0)
        {
            char * word, * brkt;
            const char * sep = ", ";
            char copy[1024];
            http_server_string_strcpy(&hdr->value, copy, sizeof(copy));
            for (word = strtok_r(copy, sep, &brkt);
                word;
                word = strtok_r(NULL, sep, &brkt))
            {
                fprintf(stderr, "word:[%s]\n", word);
                if (strcmp(word, "gzip") == 0)
                {
                    res->encoding_type = HTTP_SERVER_ENCODING_GZIP;
                    content_encoding = word;
                    break;
                }
                else if (strcmp(word, "deflate") == 0)
                {
                    res->encoding_type = HTTP_SERVER_ENCODING_DEFLATE;
                    content_encoding = word;
                    break;
                }

            }
        }
    }
    if (content_encoding)
    {
        fprintf(stderr, "set content encoding [%s]\n", content_encoding);
        r = http_server_response_set_header(res, "Content-Encoding", 16, content_encoding, strlen(content_encoding));
        if (r != HTTP_SERVER_OK)
        {
            return NULL;
        }
    }
    return res;
}

void http_server_response_free(http_server_response * res)
{
    if (res == NULL)
    {
        return;
    }
    // Free headers
    while (!TAILQ_EMPTY(&res->headers))
    {
        struct http_server_header * header = TAILQ_FIRST(&res->headers);
        TAILQ_REMOVE(&res->headers, header, headers);
        http_server_header_free(header);
    }
    if (res->encoding_type == HTTP_SERVER_ENCODING_DEFLATE
        || res->encoding_type == HTTP_SERVER_ENCODING_GZIP)
    {
        deflateEnd(res->enc.zlib_stream);
        free(res->enc.zlib_stream);
    }
    free(res);

}

int http_server_response_begin(http_server_client * client, http_server_response * res)
{
    assert(client);
    assert(!client->current_response_);
    res->client = client;
    client->current_response_ = res;
    return http_server_client_flush(client);
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

int http_server_response_write_head(http_server_response * res, int status_code)
{
    char head[1024];
    int head_len = -1;
#define XX(code, name, description) \
    case code: \
        head_len = snprintf(head, sizeof(head), "HTTP/1.1 %d %s\r\n", code, description); \
        break;
    switch (status_code)
    {
    HTTP_SERVER_ENUM_STATUS_CODES(XX)
    default:
        return HTTP_SERVER_INVALID_PARAM;
    }
    int r = http_server_client_write(res->client, head, head_len);
    return r;
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
        fprintf(stderr, "remove transfer encoding\n");
        // Remove Transfer-encoding if user sets content-length
        while (!TAILQ_EMPTY(&res->headers))
        {
            struct http_server_header * header = TAILQ_FIRST(&res->headers);
            if (http_server_header_cmp(&hdr_transferencoding, header) == 0)
            {
                TAILQ_REMOVE(&res->headers, header, headers);
                http_server_header_free(header);
                break;
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

#define my_max(a,b) (((a) > (b)) ? (a) : (b))
#define my_min(a,b) (((a) < (b)) ? (a) : (b))

static int http_server__response_transform_zlib(http_server_response * res, char * data, int size)
{
    #define CHUNK 0x4000
    #define windowBits 15
#define GZIP_ENCODING 16
    int r;
    char out[CHUNK];
    fprintf(stderr, "transform zlib(%.*s)\n", size, data);
    mz_stream * stream = res->enc.zlib_stream;
    if (!stream)
    {
        res->enc.zlib_stream = calloc(1, sizeof(mz_stream));
        stream = res->enc.zlib_stream;
        if ((r = deflateInit(stream, MZ_BEST_COMPRESSION)) < 0)
        //if ((r = deflateInit2 (stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
        //                     windowBits | GZIP_ENCODING, 8,
        //                     Z_DEFAULT_STRATEGY)) < 0)
        {
            fprintf(stderr, "deflate init error: %d\n", r);
            abort();
        }
    }
    assert(stream);
    stream->next_in = (uint8_t *)data;
    stream->avail_in = size;
    do {
        int have;
        stream->avail_out = CHUNK;
        stream->next_out = (uint8_t *)out;
        int r;
        if ((r = deflate(stream, Z_FINISH)) < 0)
        {
            fprintf(stderr, "deflate error: %d\n", r);
            abort();
        }
        have = CHUNK - stream->avail_out;
        fprintf(stderr, "compressed chunk size: %d\n", have);
        r = http_server_client_write(res->client, out, have);
        if (r != HTTP_SERVER_OK)
        {
            return r;
        }
        //fwrite (out, sizeof (char), have, stdout);
    } while(stream->avail_out == 0);
    return HTTP_SERVER_OK;
}

static int http_server__response_transform(http_server_response * res, char * data, int size)
{
    // Transform write buffer according to negotiated encoding type
    switch (res->encoding_type)
    {
    case HTTP_SERVER_ENCODING_NONE:
        return http_server_client_write(res->client, data, size);
    case HTTP_SERVER_ENCODING_DEFLATE:
    case HTTP_SERVER_ENCODING_GZIP:
        return http_server__response_transform_zlib(res, data, size);
    default:
        return HTTP_SERVER_INVALID_PARAM;
    }
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
            int r = http_server_client_write(res->client, data, data_len);
            // Remove first header from the queue
            TAILQ_REMOVE(&res->headers, header, headers);
            http_server_header_free(header);
            if (r != HTTP_SERVER_OK)
            {
                return r;
            }
        }
        assert(res->client);
        int r = http_server_client_write(res->client, "\r\n", 2);
        if (r != HTTP_SERVER_OK)
        {
            return r;
        }
        res->headers_sent = 1;
    }
    if (res->is_chunked)
    {
        // Create chunked encoding frame
        char * frame = NULL;
        int frame_length = asprintf(&frame, "%x\r\n%.*s\r\n", size, size, data);
        if (frame_length == -1)
        {
            return HTTP_SERVER_NO_MEMORY;
        }
        assert(res->client);
        int r = http_server__response_transform(res, frame, frame_length);
        if (r != HTTP_SERVER_OK)
        {
            free(frame);
            return r;
        }
        free(frame);
    }
    else
    {
        // User most probably set 'content-length' so the data is 'raw'
        if (!data || size == 0)
        {
            // Do nothing - called once will create empty response.
            return http_server_client_flush(res->client);
        }
        int r = http_server__response_transform(res, data, size);
        if (r != HTTP_SERVER_OK)
        {
            return r;
        }
    }
    return http_server_client_flush(res->client);
}

int http_server_response_printf(http_server_response * res, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    char * buffer = NULL;
    int result = vasprintf(&buffer, format, args);
    if (result == -1)
    {
        va_end(args);
        return HTTP_SERVER_NO_MEMORY;
    }
    int r = http_server_response_write(res, buffer, result);
    free(buffer);
    va_end(args);
    return r;
}
