#include <sys/resource.h>
#include "config/cmd_args.hpp"
#include "utils.hpp"
#include "arch/arch.hpp"
#include "server.hpp"
#include "side_executable.hpp"
#include "logger.hpp"

int main(int argc, char *argv[]) {
    // Before we do anything, we look at the first argument and
    // consider running a different executable (such as
    // ./rethinkdb-extract).

    consider_execve_side_executable(argc, argv, "extract");

    initialize_precise_time();
    install_generic_crash_handler();

#ifndef NDEBUG
    rlimit core_limit;
    core_limit.rlim_cur = RLIM_INFINITY;
    core_limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &core_limit);
#endif

    // Parse command line arguments
    cmd_config_t config = parse_cmd_args(argc, argv);

    // Open the log file, if necessary.
    if (config.log_file_name[0]) {
        log_file = fopen(config.log_file_name, "w");
    }

    // Initial thread message to start server
    struct server_starter_t :
        public thread_message_t
    {
        cmd_config_t *cmd_config;
        thread_pool_t *thread_pool;
        void on_thread_switch() {
            server_t *s = new server_t(cmd_config, thread_pool);
            s->do_start();
        }
    } starter;
    starter.cmd_config = &config;

    // Run the server.
    thread_pool_t thread_pool(config.n_workers);
    starter.thread_pool = &thread_pool;
    thread_pool.run(&starter);

    logINF("Server is shut down.\n");

    // Close the log file if necessary.
    if (config.log_file_name[0]) {
        fclose(log_file);
        log_file = stderr;
    }

    return 0;
}
