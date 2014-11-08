#include "http-server/http-server.h"

int main(int argc, char * argv[])
{
    int exit_code;
    http_server srv;
    // Inits data structure
    http_server_init(&srv);
    // Initializes stuff
    http_server_start(&srv);
    exit_code = http_server_run(&srv);
    // Cleans up everything
    http_server_free(&srv);
    return exit_code;
}