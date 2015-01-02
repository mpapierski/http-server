#include "http-server/http-server.h"
#include "event.h"
#include "build_config.h"
#include <strings.h>
#include <assert.h>

int Http_server_event_loop_init(http_server * srv, const char * name)
{
#if defined(HTTP_SERVER_HAVE_SELECT)
    if (strncmp(name, "select", 6) == 0)
    {
        extern struct Http_server_event_loop Http_server_event_loop_select;
        srv->event_loop_ = &Http_server_event_loop_select;
    }
    else
#endif
#if defined(HTTP_SERVER_HAVE_KQUEUE)
    if (strncmp(name, "kqueue", 6) == 0)
    {
        extern struct Http_server_event_loop Http_server_event_loop_kqueue;
        srv->event_loop_ = &Http_server_event_loop_kqueue;
    }
    else
#endif
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
