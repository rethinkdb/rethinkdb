#include "clustering/demo.hpp"
#include "clustering/cluster.hpp"
#include "clustering/serialize.hpp"
#include "clustering/rpc.hpp"
#include "store.hpp"
#include "concurrency/cond_var.hpp"
#include "conn_acceptor.hpp"
#include "memcached/memcached.hpp"
#include "logger.hpp"
#include "utils.hpp"

typedef async_mailbox_t<void(store_key_t)> query_mailbox_t;

/* demo_delegate_t */

struct demo_delegate_t : public cluster_delegate_t {
    query_mailbox_t::address_t master_address;
    demo_delegate_t(const query_mailbox_t::address_t &addr) : master_address(addr) { }
    void introduce_new_node(cluster_outpipe_t *p) {
        ::serialize(p, master_address);
    }
};

struct cluster_config_t {
    int serve_port;
    int port;
    const char *contact_host;   // NULL to start a new cluster
    int contact_port;
};

void wait_for_interrupt() {
    struct : public thread_message_t, public cond_t {
        void on_thread_switch() { pulse(); }
    } interrupt_cond;
    thread_pool_t::set_interrupt_message(&interrupt_cond);
    interrupt_cond.wait();
}

void serve(int serve_port, query_mailbox_t::address_t address) {

    struct : public get_store_t {
        query_mailbox_t::address_t address;
        get_result_t get(const store_key_t &key) {
            logINF("Sent a query.\n");
            address.call(key);
            return get_result_t();
        }
        rget_result_t rget(rget_bound_mode_t, const store_key_t&, rget_bound_mode_t, const store_key_t&) { unreachable(""); }
    } store;
    store.address = address;

    struct : public conn_acceptor_t::handler_t {
        get_store_t *store;
        void handle(tcp_conn_t *conn) {
            serve_memcache(conn, store, NULL);
        }
    } handler;
    handler.store = &store;

    conn_acceptor_t conn_acceptor(serve_port, &handler);

    logINF("Accepting connections on port %d\n", serve_port);

    wait_for_interrupt();
}

cluster_delegate_t *make_delegate(cluster_inpipe_t *pipe) {
    query_mailbox_t::address_t addr;
    ::unserialize(pipe, &addr);
    return new demo_delegate_t(addr);
}

void on_query(const store_key_t &key) {
    debugf("Got a query: %*.*s\n", (int)key.size, (int)key.size, key.contents);
}

void cluster_main(cluster_config_t config, thread_pool_t *thread_pool) {

    if (!config.contact_host) {

        logINF("Starting new cluster...\n");

        /* Start a new cluster */

        query_mailbox_t master_mailbox(&on_query);

        query_mailbox_t::address_t master_address(&master_mailbox);

        cluster_t cluster(config.port, new demo_delegate_t(master_address));

        logINF("Cluster started.\n");

        serve(config.serve_port, master_address);

    } else {

        /* Join an existing cluster */

        logINF("Joining an existing cluster.\n");

        cluster_t cluster(config.port, config.contact_host, config.contact_port, &make_delegate);

        logINF("Cluster started.\n");

        demo_delegate_t *delegate = static_cast<demo_delegate_t *>(cluster.get_delegate());

        serve(config.serve_port, delegate->master_address);
    }

    thread_pool->shutdown();
}

int run_cluster(int argc, char *argv[]) {

    struct : public thread_message_t {
        cluster_config_t config;
        thread_pool_t *thread_pool;
        void on_thread_switch() {
            coro_t::spawn(boost::bind(&cluster_main, config, thread_pool));
        }
    } starter;
    
    assert(!strcmp(argv[0], "cluster"));
    starter.config.serve_port = atoi(argv[1]);
    starter.config.port = atoi(argv[2]);
    if (argc >= 5) {
        starter.config.contact_host = argv[3];
        starter.config.contact_port = atoi(argv[4]);
    } else {
        starter.config.contact_host = NULL;
    }
    
    thread_pool_t thread_pool(8);
    starter.thread_pool = &thread_pool;
    thread_pool.run(&starter);

    return 0;
}
