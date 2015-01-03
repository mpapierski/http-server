#include "tap.h"
#include "http-server/http-server.h"
#include <strings.h>

const char * content_length = "Content-Length";

#define ENUM_HTTP_RESPONSES(XX) \
    XX(100, "Continue") \
    XX(101, "Switching Protocols") \
    XX(200, "OK") \
    XX(201, "Created") \
    XX(202, "Accepted") \
    XX(203, "Non-Authoritative Information") \
    XX(204, "No Content") \
    XX(205, "Reset Content") \
    XX(206, "Partial Content") \
    XX(300, "Multiple Choices") \
    XX(301, "Moved Permanently") \
    XX(302, "Found") \
    XX(303, "See Other") \
    XX(304, "Not Modified") \
    XX(305, "Use Proxy") \
    XX(307, "Temporary Redirect") \
    XX(400, "Bad Request") \
    XX(401, "Unauthorized") \
    XX(402, "Payment Required") \
    XX(403, "Forbidden") \
    XX(404, "Not Found") \
    XX(405, "Method Not Allowed") \
    XX(406, "Not Acceptable") \
    XX(407, "Proxy Authentication Required") \
    XX(408, "Request Timeout") \
    XX(409, "Conflict") \
    XX(410, "Gone") \
    XX(411, "Length Required") \
    XX(412, "Precondition Failed") \
    XX(413, "Request Entity Too Large") \
    XX(414, "Request-URI Too Long") \
    XX(415, "Unsupported Media Type") \
    XX(416, "Requested Range Not Satisfiable") \
    XX(417, "Expectation Failed") \
    XX(500, "Internal Server Error") \
    XX(501, "Not Implemented") \
    XX(502, "Bad Gateway") \
    XX(503, "Service Unavailable") \
    XX(504, "Gateway Timeout") \
    XX(505, "HTTP Version Not Supported")

static int unused;

void http_response_write_head_test(int code, const char * message)
{   
    http_server_response * res = http_server_response_new();
    http_server_response_write_head(res, code);
    http_server_response_write(res, NULL, 0); // empty response

    char expected[1024];
    sprintf(expected, "HTTP/1.1 %d %s\r\n", code, message);

    struct http_server_buf * buf = TAILQ_FIRST(&res->buffer);
    ok(!!buf);
    is(buf->data, expected);
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "Transfer-Encoding: chunked\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "0\r\n\r\n");
    http_server_response_free(res);
}

void test_http_response_enum()
{
    cmp_ok(HTTP_100_CONTINUE, "==", 100);
    cmp_ok(HTTP_101_SWITCHING_PROTOCOLS, "==", 101);
    cmp_ok(HTTP_200_OK, "==", 200);
    cmp_ok(HTTP_201_CREATED, "==", 201);
    cmp_ok(HTTP_202_ACCEPTED, "==", 202);
    cmp_ok(HTTP_203_NON_AUTHORITATIVE_INFORMATION, "==", 203);
    cmp_ok(HTTP_204_NO_CONTENT, "==", 204);
    cmp_ok(HTTP_205_RESET_CONTENT, "==", 205);
    cmp_ok(HTTP_206_PARTIAL_CONTENT, "==", 206);
    cmp_ok(HTTP_300_MULTIPLE_CHOICES, "==", 300);
    cmp_ok(HTTP_301_MOVED_PERMANENTLY, "==", 301);
    cmp_ok(HTTP_302_FOUND, "==", 302);
    cmp_ok(HTTP_303_SEE_OTHER, "==", 303);
    cmp_ok(HTTP_304_NOT_MODIFIED, "==", 304);
    cmp_ok(HTTP_305_USE_PROXY, "==", 305);
    cmp_ok(HTTP_307_TEMPORARY_REDIRECT, "==", 307);
    cmp_ok(HTTP_400_BAD_REQUEST, "==", 400);
    cmp_ok(HTTP_401_UNAUTHORIZED, "==", 401);
    cmp_ok(HTTP_402_PAYMENT_REQUIRED, "==", 402);
    cmp_ok(HTTP_403_FORBIDDEN, "==", 403);
    cmp_ok(HTTP_404_NOT_FOUND, "==", 404);
    cmp_ok(HTTP_405_METHOD_NOT_ALLOWED, "==", 405);
    cmp_ok(HTTP_406_NOT_ACCEPTABLE, "==", 406);
    cmp_ok(HTTP_407_PROXY_AUTHENTICATION_REQUIRED, "==", 407);
    cmp_ok(HTTP_408_REQUEST_TIMEOUT, "==", 408);
    cmp_ok(HTTP_409_CONFLICT, "==", 409);
    cmp_ok(HTTP_410_GONE, "==", 410);
    cmp_ok(HTTP_411_LENGTH_REQUIRED, "==", 411);
    cmp_ok(HTTP_412_PRECONDITION_FAILED, "==", 412);
    cmp_ok(HTTP_413_REQUEST_ENTITY_TOO_LARGE, "==", 413);
    cmp_ok(HTTP_414_REQUEST_URI_TOO_LONG, "==", 414);
    cmp_ok(HTTP_415_UNSUPPORTED_MEDIA_TYPE, "==", 415);
    cmp_ok(HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE, "==", 416);
    cmp_ok(HTTP_417_EXPECTATION_FAILED, "==", 417);
    cmp_ok(HTTP_500_INTERNAL_SERVER_ERROR, "==", 500);
    cmp_ok(HTTP_501_NOT_IMPLEMENTED, "==", 501);
    cmp_ok(HTTP_502_BAD_GATEWAY, "==", 502);
    cmp_ok(HTTP_503_SERVICE_UNAVAILABLE, "==", 503);
    cmp_ok(HTTP_504_GATEWAY_TIMEOUT, "==", 504);
    cmp_ok(HTTP_505_HTTP_VERSION_NOT_SUPPORTED, "==", 505);
}

void test_response_without_chunked_response()
{
        #define XX(code, message) do { http_response_write_head_test(code, message); } while (0);
    ENUM_HTTP_RESPONSES(XX)
    #undef XX

    test_http_response_enum();

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
    cmp_ok(http_server_response_set_header(res, "Key0", 4, "Value0", 6), "==", HTTP_SERVER_OK);
    cmp_ok(res->is_chunked, "==", 1);
    // find content-length
    ok(!TAILQ_EMPTY(&res->headers));
    // check if there is only one header and its chunked encoding
    header = TAILQ_FIRST(&res->headers);
    ok(!!header);
    is(header->key, "Transfer-Encoding");
    cmp_ok(header->key_size, "==", 17);
    is(header->value, "chunked");
    cmp_ok(header->value_size, "==", 7);
    header = TAILQ_NEXT(header, headers);
    ok(!!header);
    is(header->key, "Key0");
    cmp_ok(header->key_size, "==", 4);
    is(header->value, "Value0");
    cmp_ok(header->value_size, "==", 6);
    header = TAILQ_NEXT(header, headers);
    ok(!header);
    // write some data to flush headers
    cmp_ok(res->headers_sent, "==", 0);
    http_server_response_write_head(res, 200);
    http_server_response_write(res, "Hello world!", 12);
    http_server_response_printf(res, "Hello world %d!", 1234);
    http_server_response_end(res);
    // headers are fluhsed
    cmp_ok(res->headers_sent, "==", 1);
    ok(TAILQ_EMPTY(&res->headers));
    // check for buffer frames
    struct http_server_buf * buf = TAILQ_FIRST(&res->buffer);
    ok(!!buf);
    is(buf->data, "HTTP/1.1 200 OK\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "Transfer-Encoding: chunked\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "Key0: Value0\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "c\r\nHello world!\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "11\r\nHello world 1234!\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "0\r\n\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!buf);
    // check again if something didnt changed
    cmp_ok(res->is_chunked, "==", 1);
    // all done
    http_server_response_free(res);
}

int main(int argc, char * argv[])
{
    test_response_without_chunked_response();

    #define XX(code, message) do { http_response_write_head_test(code, message); } while (0);
    ENUM_HTTP_RESPONSES(XX)
    #undef XX

    test_http_response_enum();

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
    cmp_ok(http_server_response_set_header(res, (char*)content_length, strlen(content_length), "123", 3), "==", HTTP_SERVER_OK);
    cmp_ok(http_server_response_set_header(res, "Key0", 4, "Value0", 6), "==", HTTP_SERVER_OK);

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
    ok(!!header);
    is(header->key, "Key0");
    cmp_ok(header->key_size, "==", 4);
    is(header->value, "Value0");
    cmp_ok(header->value_size, "==", 6);
    header = TAILQ_NEXT(header, headers);
    ok(!header);
    // write some data to flush headers
    cmp_ok(res->headers_sent, "==", 0);
    http_server_response_write_head(res, 200);
    http_server_response_write(res, "Hello world!", 12);
    http_server_response_printf(res, "Hello world %d!", 1234);
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
    is(buf->data, "Key0: Value0\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "Hello world!");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!!buf);
    is(buf->data, "Hello world 1234!");
    buf = TAILQ_NEXT(buf, bufs);
    ok(!buf);
    // all done
    http_server_response_free(res);
    done_testing();
}
