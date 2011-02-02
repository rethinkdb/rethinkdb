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

typedef async_mailbox_t<void(store_key_and_buffer_t)> query_mailbox_t;

/* demo_delegate_t */

struct demo_delegate_t : public cluster_delegate_t {
    query_mailbox_t::address_t master_address;
    demo_delegate_t(const query_mailbox_t::address_t &addr) : master_address(addr) { }
    void introduce_new_node(cluster_outpipe_t *p) {
        ::serialize(p, &master_address);
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

    struct : public store_t {
        query_mailbox_t::address_t address;
        get_result_t get(store_key_t *key) {
            logINF("Sent a query.\n");
            store_key_and_buffer_t key2;
            key2.key.size = key->size;
            memcpy(key2.key.contents, key->contents, key->size);
            address.call(key2);
            return get_result_t();
        }
        rget_result_t rget(store_key_t *key1, store_key_t *key2, bool end1, bool end2, uint64_t, castime_t) { unreachable(""); }
        get_result_t get_cas(store_key_t *key, castime_t) { unreachable(""); }
        set_result_t set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t) { unreachable(""); }
        set_result_t add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t) { unreachable(""); }
        set_result_t replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t) { unreachable(""); }
        set_result_t cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, castime_t) { unreachable(""); }
        incr_decr_result_t incr(store_key_t *key, unsigned long long amount, castime_t) { unreachable(""); }
        incr_decr_result_t decr(store_key_t *key, unsigned long long amount, castime_t) { unreachable(""); }
        append_prepend_result_t append(store_key_t *key, data_provider_t *data, castime_t) { unreachable(""); }
        append_prepend_result_t prepend(store_key_t *key, data_provider_t *data, castime_t) { unreachable(""); }
        delete_result_t delete_key(store_key_t *key, repli_timestamp) { unreachable(""); }
    } store;
    store.address = address;

    struct : public conn_acceptor_t::handler_t {
        store_t *store;
        cas_generator_t cas_generator;
        void handle(tcp_conn_t *conn) {
            serve_memcache(conn, store, &cas_generator);
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

void on_query(const store_key_and_buffer_t &buf) {
    debugf("Got a query: %*.*s\n", (int)buf.key.size, (int)buf.key.size, buf.key.contents);
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
