#if !defined(HTTP_SERVER_HTTP_SERVER_INCLUDED_H_)
#define HTTP_SERVER_HTTP_SERVER_INCLUDED_H_

/**
 * Platform dependent socket type.
 */
typedef int http_server_socket_t;
#define HTTP_SERVER_INVALID_SOCKET -1

#define HTTP_SERVER_POINTER_POINT 100000
#define HTTP_SERVER_FUNCTION_POINT 200000

#if defined(HTTP_SERVER_CINIT)
#undef HTTP_SERVER_CINIT
#endif

#define HTTP_SERVER_CINIT(name, type, value) \
    HTTP_SERVER_OPT_ ## name = HTTP_SERVER_ ## type ## _POINT + value

typedef enum {
    HTTP_SERVER_CINIT(OPEN_SOCKET_FUNCTION, FUNCTION, 1),
    HTTP_SERVER_CINIT(OPEN_SOCKET_DATA, POINTER, 2),
    HTTP_SERVER_CINIT(CLOSE_SOCKET_FUNCTION, FUNCTION, 3),
    HTTP_SERVER_CINIT(CLOSE_SOCKET_DATA, POINTER, 4),
} http_server_option;

/**
 * Callback that will be called whenever http-server requests
 * new socket. At this point it is up to user to set socket's
 * purpse. It could be UDP or TCP (or some abstract type)
 * @param clientp User defined pointer
 */
typedef http_server_socket_t (*http_server_opensocket_callback)(void * clientp);

typedef struct {
    /**
     * Socket listener. This is the socket that listens for connections.
     */
    http_server_socket_t sock_listen;
    http_server_opensocket_callback opensocket_func;
    void * opensocket_data;
} http_server;

// Error codes
typedef enum {
    HTTP_SERVER_OK = 0,
} http_server_errno;

/**
 * Sets up HTTP server instance private fields.
 */
int http_server_init(http_server * srv);

/**
 * Cleans up HTTP server instance private fields.
 */
void http_server_free(http_server * srv);

/**
 * Sets options for http_server instance
 */
int http_server_setopt(http_server * srv, http_server_option opt, ...);

/**
 * Runs http server. Assumes all options are configured
 */
void http_server_run(http_server * srv);

#endif