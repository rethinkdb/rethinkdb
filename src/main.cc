
#include <signal.h>
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "utils.hpp"
#include "btree/node.hpp"
#include "worker_pool.hpp"
#include "server.hpp"
#include "btree/node.tcc"
#include "btree/internal_node.tcc"
#include "btree/leaf_node.tcc"


void term_handler(int signum) {
    // We'll naturally break out of the main loop because the accept
    // syscall will get interrupted.
}

void install_handlers() {
    int res;
    
    // Setup termination handlers
    struct sigaction action;
    bzero((char*)&action, sizeof(action));
    action.sa_handler = term_handler;
    res = sigaction(SIGTERM, &action, NULL);
    check("Could not install TERM handler", res < 0);

    bzero((char*)&action, sizeof(action));
    action.sa_handler = term_handler;
    res = sigaction(SIGINT, &action, NULL);
    check("Could not install INT handler", res < 0);
};

int main(int argc, char *argv[])
{
    // Parse command line arguments
    cmd_config_t config;
    parse_cmd_args(argc, argv, &config);

    // Setup signal handlers
    install_handlers();

    // We're using the scope here to make sure worker_pool is
    // auto-destroyed before the rest of the operations.
    {
        // Create a pool of workers
        worker_pool_t worker_pool(pthread_self(), &config);

        // Start the logger
        worker_pool.workers[LOG_WORKER]->log_writer.start(&config);

        // Start the server (in a separate thread)
        start_server(&worker_pool);

        // If we got out of start_server, we're about to shut down.
        printf("Shutting down server...\n");
    }
}
