#include "clar.h"
#include "http-server/http-server.h"

http_server_string str;

void test_strings__initialize(void)
{
	http_server_string_init(&str);
}

void test_strings__cleanup(void)
{
	http_server_string_free(&str);
}

void test_strings__append(void)
{
	int r;
	cl_assert_equal_i(str.len, 0);
	cl_assert_equal_i(str.size, 0);

	r = http_server_string_append(&str, "Hello", 5);
	cl_assert_equal_i(r, HTTP_SERVER_OK);
	cl_assert_equal_i(str.len, 5);
	cl_assert_equal_i(str.size, 6);

	r = http_server_string_append(&str, " world", 6);
	cl_assert_equal_i(r, HTTP_SERVER_OK);
	cl_assert_equal_i(str.len, 11);
	cl_assert_equal_i(str.size, 12);

	r = http_server_string_append(&str, "!", 1);
	cl_assert_equal_i(r, HTTP_SERVER_OK);
	cl_assert_equal_i(str.len, 12);
	cl_assert_equal_i(str.size, 13);

	const char * s = http_server_string_str(&str);
	cl_assert(s == str.buf);

	cl_assert_equal_s(s, "Hello world!");
	cl_assert_equal_i(s[0], 'H');
	cl_assert_equal_i(s[1], 'e');
	cl_assert_equal_i(s[2], 'l');
	cl_assert_equal_i(s[3], 'l');
	cl_assert_equal_i(s[4], 'o');
	cl_assert_equal_i(s[5], ' ');
	cl_assert_equal_i(s[6], 'w');
	cl_assert_equal_i(s[7], 'o');
	cl_assert_equal_i(s[8], 'r');
	cl_assert_equal_i(s[9], 'l');
	cl_assert_equal_i(s[10], 'd');
	cl_assert_equal_i(s[11], '!');
	cl_assert_equal_i(s[12], '\0');
}

void test_strings__clear(void)
{
	int r = http_server_string_append(&str, "Hello", 5);
	cl_assert_equal_i(str.len, 5);
	cl_assert_equal_s(http_server_string_str(&str), "Hello");
	http_server_string_clear(&str);
	r = http_server_string_append(&str, " world", 6);
	cl_assert_equal_i(r, HTTP_SERVER_OK);
	cl_assert_equal_s(http_server_string_str(&str), " world");
}
