struct Http_server_event_loop
{
    int (*init_fn)(http_server * srv);
    void (*free_fn)(http_server * srv);
    int (*run_fn)(http_server * srv);
};

/**
 * Initialize event loop by its name
 */
int Http_server_event_loop_init(http_server * srv, const char * name);

void Http_server_event_loop_free(http_server * srv);

int Http_server_event_loop_run(http_server * srv);

