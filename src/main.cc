
#include <signal.h>
#include "config/cmd_args.hpp"
#include "config/alloc.hpp"
#include "utils.hpp"
#include "arch/arch.hpp"
#include "server.hpp"

void crash_handler(int signum) {
    // We call fail here instead of letting the OS handle it because
    // fail prints a backtrace.
    fail("Internal crash detected.");
}

int main(int argc, char *argv[]) {
    
    int res;
    
    struct sigaction action;
    bzero((char*)&action, sizeof(action));
    action.sa_handler = crash_handler;
    res = sigaction(SIGSEGV, &action, NULL);
    check("Could not install SEGV handler", res < 0);
    
    // Parse command line arguments
    cmd_config_t config;
    parse_cmd_args(argc, argv, &config);
    
    // Initial CPU message to start server
    struct server_starter_t :
        public cpu_message_t
    {
        cmd_config_t *cmd_config;
        thread_pool_t *thread_pool;
        void on_cpu_switch() {
            server_t *s = new server_t(cmd_config, thread_pool);
            s->do_start();
        }
    } starter;
    starter.cmd_config = &config;
    
    // Run the server
    thread_pool_t thread_pool(config.n_workers);
    starter.thread_pool = &thread_pool;
    thread_pool.run(&starter);
    
    fprintf(stderr, "Server is shut down.\n");
    
    return 0;
}
