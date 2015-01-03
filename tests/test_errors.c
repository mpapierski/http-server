/* test_errors.c for the "Errors" suite */
#include "clar.h"
#include "http-server/http-server.h"

void test_test_errors__invalid_error(void)
{
    cl_assert_equal_i((long)http_server_errstr(123456789), 0);
}

void test_test_errors__check_messages(void)
{
    cl_assert_equal_s(http_server_errstr(HTTP_SERVER_OK), "Success");
    cl_assert_equal_s(http_server_errstr(HTTP_SERVER_SOCKET_ERROR), "Invalid socket");
    cl_assert_equal_s(http_server_errstr(HTTP_SERVER_NOTIMPL), "Not implemented error");
    cl_assert_equal_s(http_server_errstr(HTTP_SERVER_SOCKET_EXISTS), "Socket is already managed");
    cl_assert_equal_s(http_server_errstr(HTTP_SERVER_INVALID_PARAM), "Invalid parameter");
    cl_assert_equal_s(http_server_errstr(HTTP_SERVER_CLIENT_EOF), "End of file");
    cl_assert_equal_s(http_server_errstr(HTTP_SERVER_PARSER_ERROR), "Unable to parse HTTP request");	
}
