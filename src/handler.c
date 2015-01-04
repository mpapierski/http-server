#include "http-server/http-server.h"
#include <assert.h>

int http_server_handler_init(http_server_handler * handler)
{
	assert(handler);
    handler->data = NULL;
    handler->on_url = NULL;
    handler->on_url_data = NULL;
    handler->on_message_complete = NULL;
    handler->on_message_complete_data = NULL;
    handler->on_body = NULL;
    handler->on_body_data = NULL;
	return HTTP_SERVER_OK;
}
