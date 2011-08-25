#include "clustering/clustering.hpp"
#include "arch/arch.hpp"
#include <boost/bind.hpp>

struct server_starter_t : public thread_message_t {
    boost::function<void()> fun;
    thread_pool_t *thread_pool;
    void on_thread_switch() {
        coro_t::spawn_sometime(boost::bind(&server_starter_t::run, this));
    }
    void run() {
        fun();
        thread_pool->shutdown();
    }
};

void clustering_main(int port);

int run_server(int argc, char *argv[]) {

    guarantee(argc == 2, "rethinkdb-clustering expects 1 argument");
    int port = atoi(argv[1]);

    server_starter_t ss;
    thread_pool_t tp(8);
    ss.thread_pool = &tp;
    ss.fun = boost::bind(&clustering_main, port);
    tp.run(&ss);

    return 0;
}

void clustering_main(UNUSED int port) {

    // Fill this in later
}
