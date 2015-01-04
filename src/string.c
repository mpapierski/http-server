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