#include "http-server/http-server.h"
#include <stdlib.h>
#include <stdio.h>

#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <strings.h>
#include "event.h"
#include "build_config.h"

#if !defined(HTTP_SERVER_HAVE_KQUEUE)
#error "Unable to compile this file"
#endif

#define NEVENTS 64

typedef struct
{
    http_server * srv;
    // kqueue(2) handle
    int kq;
    // kqueue event watching acceptor
    struct kevent * chlist;
    // memory size allocated in chlist
    int chlist_size;
    // total "available" events in the list
    int evsize;

} Http_server_event_handler;

static int _default_opensocket_function(void * clientp);
static int _default_closesocket_function(http_server_socket_t sock, void * clientp);
static int _default_socket_function(void * clientp, http_server_socket_t sock, int flags, void * socketp);

static int Http_server_kqueue_event_loop_init(http_server * srv)
{
    // Create new default event handler
    Http_server_event_handler * ev = malloc(sizeof(Http_server_event_handler));
    if ((ev->kq = kqueue()) == -1)
    {
        perror("kqueue");
        abort();
    }
    ev->chlist = calloc(NEVENTS, sizeof(struct kevent));
    ev->chlist_size = NEVENTS;
    ev->evsize = 0;
    ev->srv = srv;
    srv->event_loop_data_ = ev;
    srv->sock_listen = HTTP_SERVER_INVALID_SOCKET;
    srv->sock_listen_data = NULL;
    srv->opensocket_func = &_default_opensocket_function;
    srv->opensocket_data = ev;
    srv->closesocket_func = &_default_closesocket_function;
    srv->closesocket_data = ev;
    srv->socket_func = &_default_socket_function;
    srv->socket_data = ev;
    return 0;
}

static void Http_server_kqueue_event_loop_free(http_server * srv)
{
    Http_server_event_handler * ev = srv->event_loop_data_;
    assert(ev);
    // Close kqueue(2) fd
    if (close(ev->kq) == -1)
    {
        perror("close");
        abort();
    }
    // Free kevent vector
    free(ev->chlist);
    // Free space for event handler
    free(ev);
}

static int _default_opensocket_function(void * clientp)
{
    Http_server_event_handler * ev = clientp;
    int s;
    int optval;
    // create default ipv4 socket for listener
    s = socket(AF_INET, SOCK_STREAM, 0);
    http_server__debug(ev->srv, 1, "open socket: %d", s);
    if (s == -1)
    {
        return HTTP_SERVER_INVALID_SOCKET;
    }
    // TODO: this part should be separated somehow
    // set SO_REUSEADDR on a socket to true (1):
    optval = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1)
    {
        perror("setsockopt");
        return HTTP_SERVER_INVALID_SOCKET;
    }

    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in sin;
    sin.sin_port = htons(5000);
    sin.sin_addr.s_addr = 0;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_family = AF_INET;

    if (bind(s, (struct sockaddr *)&sin,sizeof(struct sockaddr_in) ) == -1)
    {
        perror("bind");
        return HTTP_SERVER_INVALID_SOCKET;
    }

    if (listen(s, 128) < 0) {
        perror("listen");
        return HTTP_SERVER_INVALID_SOCKET;
    }
    // Watch event loop
    // EV_SET(&ev->chlist[evsize], s, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
    //EV_SET(&chlist[1], fileno(stdin), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
    return s;
}

static int _default_closesocket_function(http_server_socket_t sock, void * clientp)
{
    Http_server_event_handler * ev = clientp;
    http_server__debug(ev->srv, 1, "close(%d)", sock);
    // Pop event from the events vector by rewriting all events
    // from the list to the new list excluding the event related
    // with fd descriptor.
    for (int i = 0; i < ev->evsize; ++i)
    {
        if (ev->chlist[i].ident == sock)
        {
            struct kevent tmp;
            tmp = ev->chlist[ev->evsize - 1];
            ev->chlist[ev->evsize - 1] = ev->chlist[i];
            ev->chlist[i] = tmp;
            ev->evsize--;
            break;
        }
    }
    // Close the socket
    if (close(sock) == -1)
    {
        perror("close");
        abort();
    }
    return HTTP_SERVER_OK;
}

static int _default_socket_function(void * clientp, http_server_socket_t sock, int flags, void * socketp)
{
    Http_server_event_handler * ev = clientp;
    assert(ev);
    http_server * srv = ev->srv;
    assert(srv);
    // Find matching kevent structure
    struct kevent * kev = NULL;
    for (int i = 0; i < ev->evsize; ++i)
    {
        if ((long)ev->chlist[i].ident == sock)
        {
            kev = &ev->chlist[i];
            break;
        }
    }
    // If structure does not exists then resize the event list
    if (!kev)
    {
        if (ev->evsize + 1 >= ev->chlist_size)
        {
            http_server__debug(srv, 1, "RESIZE!!!");
            int current_evsize = ev->evsize;
            struct kevent * new_events = realloc(ev->chlist, sizeof(struct kevent) * (ev->chlist_size * 2));
            if (!new_events)
            {
                abort();
            }
            ev->chlist_size *= 2;
            ev->evsize += 1;
            ev->chlist = new_events;
            kev = &ev->chlist[current_evsize];
            http_server__debug(srv, 1, "evsize=%d chlist_size=%d", ev->evsize, ev->chlist_size);
        }
        else
        {
            kev = &ev->chlist[ev->evsize];
            ev->evsize++;
            assert(ev->evsize <= ev->chlist_size);
        }
    }
    // Poll for events
    if (flags & HTTP_SERVER_POLL_IN)
    {    
        EV_SET(kev, sock, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
    }
    if (flags & HTTP_SERVER_POLL_OUT)
    {    
        EV_SET(kev, sock, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
    }
    if (flags & HTTP_SERVER_POLL_REMOVE)
    {    
        EV_SET(kev, sock, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
    }
    return HTTP_SERVER_OK;
}

static int Http_server_kqueue_event_loop_run(http_server * srv)
{
    http_server__debug(srv, 1, "srv=%p", srv);
    Http_server_event_handler * ev = srv->closesocket_data;
    for (;;)
    {
        if (ev->evsize == 0)
        {
            http_server__debug(srv, 1, "no more events...");
            break;
        }
        struct kevent * evlist = calloc(ev->evsize, sizeof(struct kevent));
        assert(evlist);
        assert(ev->chlist);
        http_server__debug(srv, 1, "kq=%d chlist=%p evsize=%d", ev->kq, ev->chlist, ev->evsize);
        int nev = kevent(ev->kq, ev->chlist, ev->evsize, evlist, ev->evsize, NULL);
        http_server__debug(srv, 1, "nev=%d", nev);
        if (nev == -1)
        {
            perror("kevent");
            break;
        }
        for (int i = 0; i < nev; i++)
        {
            if (evlist[i].flags & EV_ERROR)
            {
                http_server__debug(srv, 1, "EV_ERROR: %s", strerror(evlist[i].data));
                abort();
            }
            if (evlist[i].filter == EVFILT_READ)
            {
                if (http_server_socket_action(srv, evlist[i].ident, HTTP_SERVER_POLL_IN) != HTTP_SERVER_OK)
                {
                    http_server__debug(srv, 1, "unable to read incoming data");
                    continue;
                }
            }
            if (evlist[i].filter == EVFILT_WRITE)
            {
                if (http_server_socket_action(srv, evlist[i].ident, HTTP_SERVER_POLL_OUT) != HTTP_SERVER_OK)
                {
                    http_server__debug(srv, 1, "unable to write outgoing data");
                    continue;
                }
            }
        }
        free(evlist);
    }
    return HTTP_SERVER_OK;
}

struct Http_server_event_loop Http_server_event_loop_kqueue = {
    .init_fn = &Http_server_kqueue_event_loop_init,
    .free_fn = &Http_server_kqueue_event_loop_free,
    .run_fn = &Http_server_kqueue_event_loop_run
};
