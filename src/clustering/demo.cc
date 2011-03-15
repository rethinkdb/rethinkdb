#include "clustering/demo.hpp"
#include "clustering/cluster.hpp"
#include "clustering/serialize.hpp"
#include "clustering/rpc.hpp"
#include "clustering/serialize_macros.hpp"
#include "clustering/cluster_store.hpp"
#include "clustering/dispatching_store.hpp"
#include "server/cmd_args.hpp"
#include "store.hpp"
#include "concurrency/cond_var.hpp"
#include "conn_acceptor.hpp"
#include "memcached/memcached.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "serializer/log/log_serializer.hpp"
#include "btree/slice.hpp"
#include "serializer/translator.hpp"
#include "clustering/master_map.hpp"

/* demo_delegate_t */

typedef async_mailbox_t<void(int, set_store_mailbox_t::address_t, get_store_mailbox_t::address_t)> registration_mailbox_t;

struct demo_delegate_t : public cluster_delegate_t {

    set_store_interface_mailbox_t::address_t master_store;
    get_store_mailbox_t::address_t master_get_store;
    registration_mailbox_t::address_t registration_address;

    demo_delegate_t(const set_store_interface_mailbox_t::address_t &ms, 
                    const get_store_mailbox_t::address_t &mgs, 
                    const registration_mailbox_t::address_t ra) :
        master_store(ms), master_get_store(mgs), registration_address(ra) { }

    static demo_delegate_t *construct(cluster_inpipe_t *p) {
        set_store_interface_mailbox_t::address_t master_store;
        ::unserialize(p, NULL, &master_store);
        get_store_mailbox_t::address_t master_get_store;
        ::unserialize(p, NULL, &master_get_store);
        registration_mailbox_t::address_t registration_address;
        ::unserialize(p, NULL, &registration_address);
        p->done();
        return new demo_delegate_t(master_store, master_get_store, registration_address);
    }

    int introduction_ser_size() {
        return ::ser_size(master_store) + ::ser_size(master_get_store) + ::ser_size(registration_address);
    }

    void introduce_new_node(cluster_outpipe_t *p) {
        ::serialize(p, master_store);
        ::serialize(p, master_get_store);
        ::serialize(p, registration_address);
    }
};

struct cluster_config_t {
    /* We accept memcached connections on port (31400+id). We accept cluster connections on
    port (31000+id). Our database file is ("rethinkdb_data_%d" % id). */
    int id;
    int contact_id;   // -1 for a new cluster
};

void wait_for_interrupt() {
    struct : public thread_message_t, public cond_t {
        void on_thread_switch() { pulse(); }
    } interrupt_cond;
    thread_pool_t::set_interrupt_message(&interrupt_cond);
    interrupt_cond.wait();
}

void serve(int id, demo_delegate_t *delegate) {

    cmd_config_t config;
    config.store_dynamic_config.cache.max_dirty_size = config.store_dynamic_config.cache.max_size / 10;
    char filename[200];
    snprintf(filename, sizeof(filename), "rethinkdb_data_%d", id);
    log_serializer_private_dynamic_config_t ser_config;
    ser_config.db_filename = filename;

    log_serializer_t::create(&config.store_dynamic_config.serializer, &ser_config, &config.store_static_config.serializer);
    log_serializer_t serializer(&config.store_dynamic_config.serializer, &ser_config);

    std::vector<serializer_t *> serializers;
    serializers.push_back(&serializer);
    serializer_multiplexer_t::create(serializers, 1);
    serializer_multiplexer_t multiplexer(serializers);

    btree_slice_t::create(multiplexer.proxies[0], &config.store_static_config.cache);
    btree_slice_t slice(multiplexer.proxies[0], &config.store_dynamic_config.cache);

    set_store_mailbox_t change_mailbox(&slice);
    get_store_mailbox_t get_mailbox(&slice);
    delegate->registration_address.call(get_cluster().us, &change_mailbox, &get_mailbox);

    struct : public conn_acceptor_t::handler_t {
        get_store_t *get_store;
        set_store_interface_t *set_store;
        void handle(tcp_conn_t *conn) {
            serve_memcache(conn, get_store, set_store);
        }
    } handler;
    handler.get_store = &delegate->master_get_store;
    handler.set_store = &delegate->master_store;

    int serve_port = 31400 + id;
    conn_acceptor_t conn_acceptor(serve_port, &handler);
    logINF("Accepting connections on port %d\n", serve_port);

    wait_for_interrupt();
}

void add_listener(int peer, dispatching_store_t *dispatcher, set_store_mailbox_t::address_t set_addr, get_store_mailbox_t::address_t get_addr) {
    dispatching_store_t::dispatchee_t dispatchee(peer, dispatcher, make_pair(&set_addr, &get_addr));
    struct 
        : public cond_t, public cluster_peer_t::kill_cb_t 
    {
        void on_kill() { pulse(); }
    } kill_waiter;
    kill_waiter.wait();
}

void cluster_main(cluster_config_t config, thread_pool_t *thread_pool) {

    if (config.contact_id == -1) {

        /* Start the master-components */

        dispatching_store_t dispatcher;
        registration_mailbox_t registration_mailbox(boost::bind(&add_listener, _1, &dispatcher, _2, _3));

        timestamping_set_store_interface_t timestamper(&dispatcher);
        set_store_interface_mailbox_t master_mailbox(&timestamper);

        get_store_mailbox_t master_get_mailbox(&dispatcher);

        /* Start a new cluster */

        logINF("Starting new cluster...\n");
        get_cluster().start(31000 + config.id, new demo_delegate_t(&master_mailbox, &master_get_mailbox, &registration_mailbox));
        logINF("Cluster started.\n");

        serve(config.id, static_cast<demo_delegate_t *>(get_cluster().get_delegate()));

    } else {

        /* Join an existing cluster */

        logINF("Joining an existing cluster.\n");
        get_cluster().start(31000 + config.id, "localhost", 31000 + config.contact_id, &demo_delegate_t::construct);
        logINF("Cluster started.\n");

        serve(config.id, static_cast<demo_delegate_t *>(get_cluster().get_delegate()));
    }

    unreachable("Shutdown not implemented");
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
    starter.config.id = atoi(argv[1]);
    if (argc >= 3) {
        starter.config.contact_id = atoi(argv[2]);
    } else {
        starter.config.contact_id = -1;
    }
    
    thread_pool_t thread_pool(2);
    starter.thread_pool = &thread_pool;
    thread_pool.run(&starter);

    return 0;
}
