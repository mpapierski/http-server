#include "http-server/http-server.h"
#include "event.h"
#include <strings.h>
#include <assert.h>

int Http_server_event_loop_init(http_server * srv, const char * name)
{
	// TODO: There should be several ifdefs to use best event loop on a platform
	if (strncmp(name, "select", 6) == 0)
	{
		srv->event_loop_ = &Http_server_event_loop_select;
	}
	else
	{
		// Invalid event loop
		return HTTP_SERVER_INVALID_PARAM;
	}
	assert(srv->event_loop_);
	return ((struct Http_server_event_loop *)srv->event_loop_)->init_fn(srv);
}

void Http_server_event_loop_free(http_server * srv)
{
	assert(srv->event_loop_);
	((struct Http_server_event_loop *)srv->event_loop_)->free_fn(srv);
}

int Http_server_event_loop_run(http_server * srv)
{
	return ((struct Http_server_event_loop *)srv->event_loop_)->run_fn(srv);
}
