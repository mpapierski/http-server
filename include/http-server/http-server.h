#if !defined(HTTP_SERVER_HTTP_SERVER_INCLUDED_H_)
#define HTTP_SERVER_HTTP_SERVER_INCLUDED_H_

#include <stddef.h>
#include <sys/queue.h>
#include "http_parser.h"

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
    HTTP_SERVER_CINIT(HANDLER, POINTER, 7),
    HTTP_SERVER_CINIT(HANDLER_DATA, POINTER, 8),
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

// Request handler

struct http_server_client;

/** Received a chunk of URL */
typedef int (*http_server_handler_data_cb)(struct http_server_client * client, void * data, const char * buf, size_t size);

/** Callback without args */
typedef int (*http_server_handler_cb)(struct http_server_client * client, void * data);


typedef struct http_server_handler
{
    void * data;
    http_server_handler_data_cb on_url;
    void * on_url_data;
    http_server_handler_cb on_message_complete;
    void * on_message_complete_data;
} http_server_handler;

typedef struct http_server_buf
{
    char * mem; // Memory
    char * data; // Actual data (mem > data is possible)
    int size;
    TAILQ_ENTRY(http_server_buf) bufs;
} http_server_buf;

/**
 * HTTP rresponse object
 */
typedef struct http_server_response
{
    // Owner. Could be NULL in case the response is not tied to
    // a particular client.
    struct http_server_client * client;
    TAILQ_HEAD(slisthead2, http_server_buf) buffer;
} http_server_response;

struct http_server;

/**
 * Represents single HTTP client connection.
 */
typedef struct http_server_client
{
    http_server_socket_t sock;
    void * data;
    SLIST_ENTRY(http_server_client) next;
    // private:
    http_parser_settings parser_settings_;
    http_parser parser_; // private
    http_server_handler * handler_;
    http_server_response * current_response_;
    struct http_server * server_;
    int current_flags; // current I/O poll flags
    int is_complete; // request is complete
} http_server_client;

typedef struct http_server
{
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
    /**
     * All connected clients
     */
    SLIST_HEAD(slisthead, http_server_client) clients;
    /**
     * Handler for all connections
     */
    http_server_handler * handler_;
    /**
     * Current HTTP response
     */
    http_server_response * response_;
} http_server;

#define HTTP_SERVER_ENUM_ERROR_CODES(XX) \
    XX(OK, 0, "Success") \
    XX(SOCKET_ERROR, 1, "Invalid socket") \
    XX(NOTIMPL, 2, "Not implemented error") \
    XX(SOCKET_EXISTS, 3, "Socket is already managed") \
    XX(INVALID_PARAM, 4, "Invalid parameter") \
    XX(CLIENT_EOF, 5, "End of file") \
    XX(PARSER_ERROR, 6, "Unable to parse HTTP request")

#define HTTP_SERVER_ENUM_ERRNO(name, val, descr) \
    HTTP_SERVER_ ## name = val,

// Error codes
typedef enum {
    HTTP_SERVER_ENUM_ERROR_CODES(HTTP_SERVER_ENUM_ERRNO)
} http_server_errno;

#undef HTTP_SERVER_ENUM_ERRNO

// Poll for reading
#define HTTP_SERVER_POLL_IN 1<<1
// Poll for writing
#define HTTP_SERVER_POLL_OUT 1<<2
// Poll to be removed for further polling
#define HTTP_SERVER_POLL_REMOVE 1<<3

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
int http_server_start(http_server * srv);

/**
 * Blocks and serves connections
 */
int http_server_run(http_server * srv);

/**
 * Assigns pointer to a socket
 * @param sock Socket
 * @param data Data
 */
int http_server_assign(http_server * srv, http_server_socket_t sock, void * data);

/**
 * Adds new client to manage.
 * @param srv Server
 * @param sock Socket
 */
int http_server_add_client(http_server * srv, http_server_socket_t sock);

/**
 * Remove client from the list
 * @param srv Server
 * @param sock Socket
 */
int http_server_pop_client(http_server * srv, http_server_socket_t sock);

/**
 * Perform action on a socket. Usually called after receiving some async event.
 * @param srv Server instance
 * @param socket Socket object
 * @param flags Flags (mix of flags: HTTP_SERVER_POLL_IN|HTTP_SERVER_POLL_OUT)
 */
int http_server_socket_action(http_server * srv, http_server_socket_t socket, int flags);

/**
 * Create new HTTP client instance
 */
http_server_client * http_server_new_client(http_server * server, http_server_socket_t sock, http_server_handler * handler);

/**
 * Feeds client using chunk of datad    
 */
int http_server_perform_client(http_server_client * client, const char * at, size_t size);

/**
 * Calls for I/O poll on a client
 * @private
 */
int http_server_poll_client(http_server_client * client, int flags);

// Response API
#define HTTP_SERVER_ENUM_STATUS_CODES(XX) \
    XX(200, OK, "OK")

typedef enum
{
#define XX(status_code, name, description) HTTP_ ## name = status_code,
    HTTP_SERVER_ENUM_STATUS_CODES(XX)
#undef XX
} http_server_status_code;

/**
 * Creates new response object
 */
http_server_response * http_server_response_new();

/**
 * Queues response to the client.
 */
int http_server_response_begin(http_server_client * client, http_server_response * res);

/**
 * Response is done.
 */
int http_server_response_end(http_server_response * client);

/**
 * Start sending the response.
 * User should not call this directly.
 */
int http_server_response__flush(http_server_response * res);

/**
 * Write response header
 */
int http_server_response_write_head(http_server_response * res, int status_code);

/**
 * Sets header in response
 */
int http_server_response_set_header(http_server_response * res, char * name, int namelen, char * value, int valuelen);

/**
 * Write some data to the responses
 * @param res Response
 * @param data Data
 * @param size Size
 */
int http_server_response_write(http_server_response * res, char * data, int size);

#endif
