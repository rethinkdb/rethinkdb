#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "extract/filewalk.hpp"
#include "alloc/gnew.hpp"
#include "config/cmd_args.hpp"
#include "errors.hpp"
#include "logger.hpp"



void checkmain_usage(const char *name) {
    printf("Usage:\n");
    printf("\t%s dbfile dumpfile\n", name);

    exit(-1);
}


void checkmain(int argc, char **argv) {
    if (argc != 3) {
        checkmain_usage(argv[0]);
    }

    const char *db_filepath = argv[1];
    const char *dump_filepath = argv[2];

    dumpfile(db_filepath, dump_filepath);
}

void filecheck_crash_handler(int signum) {
    fail("Internal crash detected.");
}

struct blocking_runner_t {
    virtual void run() = 0;
};

cmd_config_t *make_fake_config() {
    static cmd_config_t fake_config;
    init_config(&fake_config);
    return &fake_config;
}

struct run_in_loggers_fsm_t : public log_controller_t::ready_callback_t,
                              public log_controller_t::shutdown_callback_t {
    run_in_loggers_fsm_t(thread_pool_t *pool, blocking_runner_t *runner) : pool(pool), runner(runner), controller(make_fake_config()) { }
    ~run_in_loggers_fsm_t() {
        gdelete(runner);
    }

    void start() {
        if (controller.start(this)) {
            on_logger_ready();
        }
    }

    void on_logger_ready() {
        runner->run();

        if (controller.shutdown(this)) {
            on_logger_shutdown();
        }
    }

    void on_logger_shutdown() {
        pool->shutdown();
        gdelete(this);
    }

private:
    thread_pool_t *pool;
    blocking_runner_t *runner;
    log_controller_t controller;
};



struct checkmain_runner : public blocking_runner_t {
    int argc;
    char **argv;
    checkmain_runner(int argc, char **argv) : argc(argc), argv(argv) { }
    void run() {
        checkmain(argc, argv);
    }
};

int main(int argc, char **argv) {

    int res;

    struct sigaction action;
    bzero((char*)&action, sizeof(action));
    action.sa_handler = filecheck_crash_handler;
    res = sigaction(SIGSEGV, &action, NULL);
    check("Could not install SEGV handler", res < 0);

    // Initial CPU message to start server
    struct server_starter_t :
        public cpu_message_t
    {
        int argc;
        char **argv;
        thread_pool_t *pool;
        void on_cpu_switch() {
            checkmain_runner *runner = gnew<checkmain_runner>(argc, argv);
            run_in_loggers_fsm_t *fsm = gnew<run_in_loggers_fsm_t>(pool, runner);
            fsm->start();
        }
    } starter;

    starter.argc = argc;
    starter.argv = argv;
    
    // Run the server
    thread_pool_t thread_pool(make_fake_config()->n_workers);
    starter.pool = &thread_pool;
    thread_pool.run(&starter);

    return 0;
}
