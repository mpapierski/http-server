#include "clar.h"
#include "http-server/http-server.h"
#include <stdio.h>
#include <strings.h>

const char * content_length = "Content-Length";

void http_response_write_head_test(int code, const char * message)
{   
    http_server_response * res = http_server_response_new();
    http_server_response_write_head(res, code);
    http_server_response_write(res, NULL, 0); // empty response

    char expected[1024];
    sprintf(expected, "HTTP/1.1 %d %s\r\n", code, message);

    struct http_server_buf * buf = TAILQ_FIRST(&res->buffer);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, expected);
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "Transfer-Encoding: chunked\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "0\r\n\r\n");
    http_server_response_free(res);
}

void test_response__write_head(void)
{
    http_response_write_head_test(100, "Continue");
    http_response_write_head_test(101, "Switching Protocols");
    http_response_write_head_test(200, "OK");
    http_response_write_head_test(201, "Created");
    http_response_write_head_test(202, "Accepted");
    http_response_write_head_test(203, "Non-Authoritative Information");
    http_response_write_head_test(204, "No Content");
    http_response_write_head_test(205, "Reset Content");
    http_response_write_head_test(206, "Partial Content");
    http_response_write_head_test(300, "Multiple Choices");
    http_response_write_head_test(301, "Moved Permanently");
    http_response_write_head_test(302, "Found");
    http_response_write_head_test(303, "See Other");
    http_response_write_head_test(304, "Not Modified");
    http_response_write_head_test(305, "Use Proxy");
    http_response_write_head_test(307, "Temporary Redirect");
    http_response_write_head_test(400, "Bad Request");
    http_response_write_head_test(401, "Unauthorized");
    http_response_write_head_test(402, "Payment Required");
    http_response_write_head_test(403, "Forbidden");
    http_response_write_head_test(404, "Not Found");
    http_response_write_head_test(405, "Method Not Allowed");
    http_response_write_head_test(406, "Not Acceptable");
    http_response_write_head_test(407, "Proxy Authentication Required");
    http_response_write_head_test(408, "Request Timeout");
    http_response_write_head_test(409, "Conflict");
    http_response_write_head_test(410, "Gone");
    http_response_write_head_test(411, "Length Required");
    http_response_write_head_test(412, "Precondition Failed");
    http_response_write_head_test(413, "Request Entity Too Large");
    http_response_write_head_test(414, "Request-URI Too Long");
    http_response_write_head_test(415, "Unsupported Media Type");
    http_response_write_head_test(416, "Requested Range Not Satisfiable");
    http_response_write_head_test(417, "Expectation Failed");
    http_response_write_head_test(500, "Internal Server Error");
    http_response_write_head_test(501, "Not Implemented");
    http_response_write_head_test(502, "Bad Gateway");
    http_response_write_head_test(503, "Service Unavailable");
    http_response_write_head_test(504, "Gateway Timeout");
    http_response_write_head_test(505, "HTTP Version Not Supported");
}

void test_test_response__enum(void)
{
    cl_assert_equal_i(HTTP_100_CONTINUE, 100);
    cl_assert_equal_i(HTTP_101_SWITCHING_PROTOCOLS, 101);
    cl_assert_equal_i(HTTP_200_OK, 200);
    cl_assert_equal_i(HTTP_201_CREATED, 201);
    cl_assert_equal_i(HTTP_202_ACCEPTED, 202);
    cl_assert_equal_i(HTTP_203_NON_AUTHORITATIVE_INFORMATION, 203);
    cl_assert_equal_i(HTTP_204_NO_CONTENT, 204);
    cl_assert_equal_i(HTTP_205_RESET_CONTENT, 205);
    cl_assert_equal_i(HTTP_206_PARTIAL_CONTENT, 206);
    cl_assert_equal_i(HTTP_300_MULTIPLE_CHOICES, 300);
    cl_assert_equal_i(HTTP_301_MOVED_PERMANENTLY, 301);
    cl_assert_equal_i(HTTP_302_FOUND, 302);
    cl_assert_equal_i(HTTP_303_SEE_OTHER, 303);
    cl_assert_equal_i(HTTP_304_NOT_MODIFIED, 304);
    cl_assert_equal_i(HTTP_305_USE_PROXY, 305);
    cl_assert_equal_i(HTTP_307_TEMPORARY_REDIRECT, 307);
    cl_assert_equal_i(HTTP_400_BAD_REQUEST, 400);
    cl_assert_equal_i(HTTP_401_UNAUTHORIZED, 401);
    cl_assert_equal_i(HTTP_402_PAYMENT_REQUIRED, 402);
    cl_assert_equal_i(HTTP_403_FORBIDDEN, 403);
    cl_assert_equal_i(HTTP_404_NOT_FOUND, 404);
    cl_assert_equal_i(HTTP_405_METHOD_NOT_ALLOWED, 405);
    cl_assert_equal_i(HTTP_406_NOT_ACCEPTABLE, 406);
    cl_assert_equal_i(HTTP_407_PROXY_AUTHENTICATION_REQUIRED, 407);
    cl_assert_equal_i(HTTP_408_REQUEST_TIMEOUT, 408);
    cl_assert_equal_i(HTTP_409_CONFLICT, 409);
    cl_assert_equal_i(HTTP_410_GONE, 410);
    cl_assert_equal_i(HTTP_411_LENGTH_REQUIRED, 411);
    cl_assert_equal_i(HTTP_412_PRECONDITION_FAILED, 412);
    cl_assert_equal_i(HTTP_413_REQUEST_ENTITY_TOO_LARGE, 413);
    cl_assert_equal_i(HTTP_414_REQUEST_URI_TOO_LONG, 414);
    cl_assert_equal_i(HTTP_415_UNSUPPORTED_MEDIA_TYPE, 415);
    cl_assert_equal_i(HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE, 416);
    cl_assert_equal_i(HTTP_417_EXPECTATION_FAILED, 417);
    cl_assert_equal_i(HTTP_500_INTERNAL_SERVER_ERROR, 500);
    cl_assert_equal_i(HTTP_501_NOT_IMPLEMENTED, 501);
    cl_assert_equal_i(HTTP_502_BAD_GATEWAY, 502);
    cl_assert_equal_i(HTTP_503_SERVICE_UNAVAILABLE, 503);
    cl_assert_equal_i(HTTP_504_GATEWAY_TIMEOUT, 504);
    cl_assert_equal_i(HTTP_505_HTTP_VERSION_NOT_SUPPORTED, 505);
}

void test_test_response__without_chunked_response(void)
{
    //#define XX(code, message) do { http_response_write_head_test(code, message); } while (0);
    //ENUM_HTTP_RESPONSES(XX)
    //#undef XX
//
    //test_http_response_enum();

    http_server_response * res = http_server_response_new();
    // by default http response is chunked
    cl_assert_equal_i(res->is_chunked, 1);
    cl_assert(!TAILQ_EMPTY(&res->headers));
    // check if there is only one header and its chunked encoding
    struct http_server_header * header = TAILQ_FIRST(&res->headers);
    cl_assert(!!header);
    cl_assert_equal_s(http_server_string_str(&header->field), "Transfer-Encoding");
    cl_assert_equal_s(http_server_string_str(&header->value), "chunked");
    header = TAILQ_NEXT(header, headers);
    cl_assert(!header); // no more headers
    // now siwtch to content-length and chunked encoding should be gone
    cl_assert_equal_i(http_server_response_set_header(res, "Key0", 4, "Value0", 6), HTTP_SERVER_OK);
    cl_assert_equal_i(res->is_chunked, 1);
    // find content-length
    cl_assert(!TAILQ_EMPTY(&res->headers));
    // check if there is only one header and its chunked encoding
    header = TAILQ_FIRST(&res->headers);
    cl_assert(!!header);
    cl_assert_equal_s(http_server_string_str(&header->field), "Transfer-Encoding");
    cl_assert_equal_s(http_server_string_str(&header->value), "chunked");
    header = TAILQ_NEXT(header, headers);
    cl_assert(!!header);
    cl_assert_equal_s(http_server_string_str(&header->field), "Key0");
    cl_assert_equal_s(http_server_string_str(&header->value), "Value0");
    header = TAILQ_NEXT(header, headers);
    cl_assert(!header);
    // write some data to flush headers
    cl_assert_equal_i(res->headers_sent, 0);
    http_server_response_write_head(res, 200);
    http_server_response_write(res, "Hello world!", 12);
    http_server_response_printf(res, "Hello world %d!", 1234);
    http_server_response_end(res);
    // headers are fluhsed
    cl_assert_equal_i(res->headers_sent, 1);
    cl_assert(TAILQ_EMPTY(&res->headers));
    // check for buffer frames
    struct http_server_buf * buf = TAILQ_FIRST(&res->buffer);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "HTTP/1.1 200 OK\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "Transfer-Encoding: chunked\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "Key0: Value0\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "c\r\nHello world!\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "11\r\nHello world 1234!\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "0\r\n\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!buf);
    // check again if something didnt changed
    cl_assert_equal_i(res->is_chunked, 1);
    // all done
    http_server_response_free(res);
}

void test_test_response__with_content_length(void)
{
    //#define XX(code, message) do { http_response_write_head_test(code, message); } while (0);
    //ENUM_HTTP_RESPONSES(XX)
    //#undef XX
//
    //test_http_response_enum();

    http_server_response * res = http_server_response_new();
    // by default http response is chunked
    cl_assert_equal_i(res->is_chunked, 1);
    cl_assert(!TAILQ_EMPTY(&res->headers));
    // check if there is only one header and its chunked encoding
    struct http_server_header * header = TAILQ_FIRST(&res->headers);
    cl_assert(!!header);
    cl_assert_equal_s(http_server_string_str(&header->field), "Transfer-Encoding");
    cl_assert_equal_s(http_server_string_str(&header->value), "chunked");
    header = TAILQ_NEXT(header, headers);
    cl_assert(!header); // no more headers
    // now siwtch to content-length and chunked encoding should be gone
    cl_assert_equal_i(http_server_response_set_header(res, (char*)content_length, strlen(content_length), "123", 3), HTTP_SERVER_OK);
    cl_assert_equal_i(http_server_response_set_header(res, "Key0", 4, "Value0", 6), HTTP_SERVER_OK);

    cl_assert_equal_i(res->is_chunked, 0);
    // find content-length
    cl_assert(!TAILQ_EMPTY(&res->headers));
    // check if there is only one header and its chunked encoding
    header = TAILQ_FIRST(&res->headers);
    cl_assert(!!header);
    cl_assert_equal_s(http_server_string_str(&header->field), content_length);
    cl_assert_equal_s(http_server_string_str(&header->value), "123");
    header = TAILQ_NEXT(header, headers);
    cl_assert(!!header);
    cl_assert_equal_s(http_server_string_str(&header->field), "Key0");
    cl_assert_equal_s(http_server_string_str(&header->value), "Value0");
    header = TAILQ_NEXT(header, headers);
    cl_assert(!header);
    // write some data to flush headers
    cl_assert_equal_i(res->headers_sent, 0);
    http_server_response_write_head(res, 200);
    http_server_response_write(res, "Hello world!", 12);
    http_server_response_printf(res, "Hello world %d!", 1234);
    // headers are fluhsed
    cl_assert_equal_i(res->headers_sent, 1);
    cl_assert(TAILQ_EMPTY(&res->headers));
    // check for buffer frames
    struct http_server_buf * buf = TAILQ_FIRST(&res->buffer);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "HTTP/1.1 200 OK\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "Content-Length: 123\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "Key0: Value0\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "\r\n");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "Hello world!");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!!buf);
    cl_assert_equal_s(buf->data, "Hello world 1234!");
    buf = TAILQ_NEXT(buf, bufs);
    cl_assert(!buf);
    // all done
    http_server_response_free(res);
}
