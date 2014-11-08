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
    HTTP_SERVER_CINIT(SOCKET_FUNCTION, FUNCTION, 3),
    HTTP_SERVER_CINIT(SOCKET_DATA, POINTER, 4),
    HTTP_SERVER_CINIT(CLOSE_SOCKET_FUNCTION, FUNCTION, 5),
    HTTP_SERVER_CINIT(CLOSE_SOCKET_DATA, POINTER, 6),
} http_server_option;

/**
 * Callback that will be called whenever http-server requests
 * new socket. At this point it is up to user to set socket's
 * purpse. It could be UDP or TCP (or some abstract type)
 * @param clientp User defined pointer
 */
typedef http_server_socket_t (*http_server_opensocket_callback)(void * clientp);

/**
 * Called when http-server requests some poll.
 * @param clientp Client pointer
 * @param sock Socket
 * @param flags Some mix of flags {HTTP_SERVER_POLL_IN,HTTP_SERVER_POLL_OUT}
 * @param socketp Some custom data assigned to socket (could be NULL)
 */
typedef int (*http_server_socket_callback)(void * clientp, http_server_socket_t sock, int flags, void * socketp);

typedef struct {
    /**
     * Socket listener. This is the socket that listens for connections.
     */
    http_server_socket_t sock_listen;
    /**
     * This is what user assigns
     */
    void * sock_listen_data;
    /**
     * Callback that user provides and should return new low-level
     * socket.
     */
    http_server_opensocket_callback opensocket_func;
    /**
     * Custom data pointer that user provides.
     */
    void * opensocket_data;
    /**
     * Called when http-server internals wants some async action
     */
    http_server_socket_callback socket_func;
    /**
     * Custom data pointer for `socket_func` that user provides
     */
    void * socket_data;
} http_server;

// Error codes
typedef enum {
    HTTP_SERVER_OK = 0,
    HTTP_SERVER_SOCKET_ERROR, // Invalid socket error
    HTTP_SERVER_NOTIMPL, // Not implemented error
} http_server_errno;

// Poll for reading
#define HTTP_SERVER_POLL_IN 0
// Poll for writing
#define HTTP_SERVER_POLL_OUT 1

/**
 * Sets up HTTP server instance private fields.
 */
int http_server_init(http_server * srv);

/**
 * Cleans up HTTP server instance private fields.
 */
void http_server_free(http_server * srv);

/**
 * String description of error.
 * @param e Error code
 * @return Description as string
 */
char * http_server_errstr(http_server_errno e);

/**
 * Sets options for http_server instance
 */
int http_server_setopt(http_server * srv, http_server_option opt, ...);

/**
 * Starts http server. Assumes all options are configured. This will
 * create sockets.
 */
int http_server_run(http_server * srv);

/**
 * Assigns pointer to a socket
 * @param sock Socket
 * @param data Data
 */
int http_server_assign(http_server * srv, http_server_socket_t sock, void * data);

#endif