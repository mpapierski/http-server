#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void http_server_string_init(http_server_string * str)
{
    assert(str);
    str->buf = NULL;
    str->len = 0;
    str->size = 0;
}

void http_server_string_free(http_server_string * str)
{
    assert(str);
    free(str->buf);
}

int http_server_string_append(http_server_string * str, const char * data, int size)
{
    assert(str);
    if (str->len + size >= str->size)
    {
        // Just to save one byte when there are multiple `append` operations
        // we can calculate the real size of the string.
        int real_size = str->size - 1;
        if (real_size < 0)
        {
            real_size = 0;
        }
        // Resize memory
        char * new_buf = realloc(str->buf, sizeof(char) * (real_size + size + 1));
        if (!new_buf)
        {
            return HTTP_SERVER_NO_MEMORY;
        }
        str->buf = new_buf;
        str->size = real_size + size + 1;
    }
    assert(str->buf);
    // Copy data and remember to put NULL byte at the end
    memcpy(str->buf + str->len, data, size);
    str->buf[str->len + size] = '\0';
    str->len += size;
    return HTTP_SERVER_OK;
}

const char * http_server_string_str(http_server_string * str)
{
    return str->buf;
}

void http_server_string_clear(http_server_string * str)
{
    if (!str)
    {
        return;
    }
    free(str->buf);
    str->buf = NULL;
    str->len = 0;
    str->size = 0;
}

void http_server_string_move(http_server_string * str1, http_server_string * str2)
{
    if (str2->buf)
    {
        http_server_string_free(str2);
    }
    str2->buf = str1->buf;
    str1->buf = NULL;
    str2->len = str1->len;
    str1->len = 0;
    str2->size = str1->size;
    str1->size = 0;
}

void http_server_string_strcpy(http_server_string * str, char * buf, int size)
{
    if (!str
        || !buf
        || size == 0)
    {
        return;
    }
    strncpy(buf, str->buf, size);
}

int http_server_string_length(http_server_string * str)
{
    if (!str)
    {
        return 0;
    }
    return str->len;    
}

int http_server_string_assign(http_server_string * src, http_server_string * dst)
{
    if (!src
        || !dst)
    {
        return HTTP_SERVER_INVALID_PARAM;
    }
    // Dest string should be empty. All data will be overwritten.
    http_server_string_clear(dst);
    // Write `s
    const char * src_str = http_server_string_str(src);
    int src_len = http_server_string_length(src);
    return http_server_string_append(src, src_str, src_len);
}