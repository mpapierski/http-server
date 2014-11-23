#include "tap.h"
#include "http-server/http-server.h"
#include <strings.h>

const char * content_length = "Content-Length";

int main(int argc, char * argv[])
{
    http_server_response * res = http_server_response_new();
    // by default http response is chunked
    cmp_ok(res->is_chunked, "==", 1);
    ok(!TAILQ_EMPTY(&res->headers));
    // check if there is only one header and its chunked encoding
    struct http_server_header * header = TAILQ_FIRST(&res->headers);
    ok(!!header);
    is(header->key, "Transfer-Encoding");
    cmp_ok(header->key_size, "==", 17);
    is(header->value, "chunked");
    cmp_ok(header->value_size, "==", 7);
    header = TAILQ_NEXT(header, headers);
    ok(!header); // no more headers
    // now siwtch to content-length and chunked encoding should be gone
    http_server_response_set_header(res, (char*)content_length, strlen(content_length), "123", 3);
    cmp_ok(res->is_chunked, "==", 0);
    // find content-length
    ok(!TAILQ_EMPTY(&res->headers));
    // check if there is only one header and its chunked encoding
    header = TAILQ_FIRST(&res->headers);
    ok(!!header);
    is(header->key, content_length);
    cmp_ok(header->key_size, "==", strlen(content_length));
    is(header->value, "123");
    cmp_ok(header->value_size, "==", 3);
    header = TAILQ_NEXT(header, headers);
    ok(!header);
    // write some data to flush headers
    cmp_ok(res->headers_sent, "==", 0);
    http_server_response_write_head(res, 200);
    http_server_response_write(res, "Hello world!", 12);
    // headers are fluhsed
    cmp_ok(res->headers_sent, "==", 1);
    ok(TAILQ_EMPTY(&res->headers));
    // check for buffer frames
    struct http_server_buf * buf = TAILQ_FIRST(&res->buffer);
    ok(!!buf);
    is(buf->data, "HTTP/1.1 200 OK\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "Content-Length: 123\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "Hello world!");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!buf);
    // all done
    http_server_response_free(res);
    done_testing();
}
