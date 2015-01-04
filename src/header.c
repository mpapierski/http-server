#include "http-server/http-server.h"
#include <stdlib.h>

struct http_server_header * http_server_header_new()
{
	struct http_server_header * header = malloc(sizeof(struct http_server_header));
	if (!header)
	{
		return NULL;
	}
	http_server_string_init(&header->field);
	http_server_string_init(&header->value);
	return header;
}

void http_server_header_free(struct http_server_header * header)
{
	if (!header)
	{
		return;
	}
	http_server_string_free(&header->field);
	http_server_string_free(&header->value);
	free(header);
}
