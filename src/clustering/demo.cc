#include "clustering/demo.hpp"
#include "clustering/cluster.hpp"
#include "store.hpp"
#include "concurrency/cond_var.hpp"
#include "conn_acceptor.hpp"
#include "memcached/memcached.hpp"
#include "logger.hpp"
#include "utils.hpp"

/* intro_message_t */

struct intro_message_t : public cluster_message_t {

    static struct format_t : public cluster_message_format_t {
        void serialize(cluster_outpipe_t *pipe, unique_ptr_t<cluster_message_t> msg) const {
            unique_ptr_t<intro_message_t> m = static_pointer_cast<intro_message_t>(msg);
            pipe->write_address(&m->address);
        }
        unique_ptr_t<cluster_message_t> unserialize(cluster_inpipe_t *pipe) const {
            unique_ptr_t<intro_message_t> m(new intro_message_t);
            pipe->read_address(&m->address);
            return m;
        }
        format_t() : cluster_message_format_t(1) { }
    } format;

    intro_message_t() :
        cluster_message_t(&format) { }
    intro_message_t(const cluster_address_t &address) :
        cluster_message_t(&format), address(address) { }

    cluster_address_t address;
};

intro_message_t::format_t intro_message_t::format;

/* demo_delegate_t */

struct demo_delegate_t : public cluster_delegate_t {

    cluster_address_t master_address;

    demo_delegate_t(const cluster_address_t &a) : master_address(a) { }

    unique_ptr_t<cluster_message_t> introduce_new_node() {
        return new intro_message_t(master_address);
    }
};

/* query_message_t */

struct query_message_t : public cluster_message_t {

    static struct format_t : public cluster_message_format_t {
        void serialize(cluster_outpipe_t *pipe, unique_ptr_t<cluster_message_t> msg) const {
            unique_ptr_t<query_message_t> m = static_pointer_cast<query_message_t>(msg);
            pipe->write(&m->buf.key.size, sizeof(m->buf.key.size));
            pipe->write(m->buf.key.contents, m->buf.key.size);
        }
        unique_ptr_t<cluster_message_t> unserialize(cluster_inpipe_t *pipe) const {
            unique_ptr_t<query_message_t> m(new query_message_t);
            pipe->read(&m->buf.key.size, sizeof(m->buf.key.size));
            pipe->read(m->buf.key.contents, m->buf.key.size);
            return m;
        }
        format_t() : cluster_message_format_t(2) { }
    } format;

    query_message_t() :
        cluster_message_t(&format) { }
    query_message_t(store_key_t *k) :
        cluster_message_t(&format)
    {
        buf.key.size = k->size;
        memcpy(buf.key.contents, k->contents, k->size);
    }

    store_key_and_buffer_t buf;
};

query_message_t::format_t query_message_t::format;

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

void serve(int serve_port, cluster_address_t address) {

    struct : public store_t {
        cluster_address_t address;
        get_result_t get(store_key_t *key) {\
            logINF("Sent a query.\n");
            address.send(new query_message_t(key));
            return get_result_t();
        }
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

    conn_acceptor_t conn_acceptor(serve_port, &handler, false);

    logINF("Accepting connections on port %d\n", serve_port);

    wait_for_interrupt();
}

cluster_delegate_t *make_delegate(unique_ptr_t<cluster_message_t> msg) {
    unique_ptr_t<intro_message_t> m = static_pointer_cast<intro_message_t>(msg);
    cluster_delegate_t *delegate = new demo_delegate_t(m->address);
    return delegate;
}

void cluster_main(cluster_config_t config, thread_pool_t *thread_pool) {

    if (!config.contact_host) {

        logINF("Starting new cluster...\n");

        /* Start a new cluster */

        struct : public cluster_mailbox_t {
            virtual void run(unique_ptr_t<cluster_message_t> msg) {
                unique_ptr_t<query_message_t> m = static_pointer_cast<query_message_t>(msg);
                debugf("Got a query: %*.*s\n", (int)m->buf.key.size, (int)m->buf.key.size,
                    m->buf.key.contents);
            }
        } master_mailbox;

        cluster_t cluster(config.port, new demo_delegate_t(master_mailbox.get_address()));

        logINF("Cluster started.\n");

        serve(config.serve_port, master_mailbox.get_address());

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
            coro_t::spawn(&cluster_main, config, thread_pool);
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
