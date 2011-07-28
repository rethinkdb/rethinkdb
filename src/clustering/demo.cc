#include "buffer_cache/buffer_cache.hpp"
#include "clustering/demo.hpp"
#include "rpc/core/cluster.hpp"
#include "rpc/serialize/serialize.hpp"
#include "rpc/rpc.hpp"
#include "rpc/serialize/serialize_macros.hpp"
#include "rpc/council.hpp"
#include "clustering/cluster_store.hpp"
#include "clustering/dispatching_store.hpp"
#include "server/cmd_args.hpp"
#include "store.hpp"
#include "concurrency/cond_var.hpp"
//#include "protocol/memcached/tcp_conn.hpp"
#include "protocol/protocol.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "serializer/log/log_serializer.hpp"
#include "btree/slice.hpp"
#include "serializer/translator.hpp"
#include "clustering/master_map.hpp"
#include "server/control.hpp"
#include "arch/os_signal.hpp"
#include "clustering/council_delegate.hpp"
#include "clustering/map_council.hpp"
#include "clustering/routing.hpp"

/* demo_delegate_t */

typedef async_mailbox_t<void(int, set_store_mailbox_t::address_t, get_store_mailbox_t::address_t)> registration_mailbox_t;

typedef map_council_t<int, int> int_map_council_t;

struct demo_delegate_t : public cluster_delegate_t {

    set_store_interface_mailbox_t::address_t master_store;
    get_store_mailbox_t::address_t master_get_store;
    registration_mailbox_t::address_t registration_address;

    int_map_council_t demo_map_council;

    routing::routing_map_t routing_map;

    demo_delegate_t(const set_store_interface_mailbox_t::address_t &ms, 
                    const get_store_mailbox_t::address_t &mgs, 
                    const registration_mailbox_t::address_t &ra) 
        : master_store(ms), master_get_store(mgs), registration_address(ra),
          map_council_control("map-council", "access the map council", &demo_map_council)
    { }

    demo_delegate_t(const set_store_interface_mailbox_t::address_t &ms, 
                    const get_store_mailbox_t::address_t &mgs, 
                    const registration_mailbox_t::address_t &ra,
                    const int_map_council_t::address_t &mc_addr,
                    const routing::routing_map_t::address_t &rm_addr) 
        : master_store(ms), master_get_store(mgs), registration_address(ra),
          demo_map_council(mc_addr),
          routing_map(rm_addr),
          map_council_control("map-council", "access the map council", &demo_map_council)
    { }

    static demo_delegate_t *construct(cluster_inpipe_t *p, boost::function<void()> done) {
        set_store_interface_mailbox_t::address_t master_store;
        ::unserialize(p, NULL, &master_store);
        get_store_mailbox_t::address_t master_get_store;
        ::unserialize(p, NULL, &master_get_store);
        registration_mailbox_t::address_t registration_address;
        ::unserialize(p, NULL, &registration_address);
        map_council_t<int, int>::address_t demo_map_council_addr;
        ::unserialize(p, NULL, &demo_map_council_addr);
        routing::routing_map_t::address_t routing_map_addr;
        ::unserialize(p, NULL, &routing_map_addr);
        done();
        return new demo_delegate_t(master_store, master_get_store, registration_address, demo_map_council_addr, routing_map_addr);
    }

    void introduce_new_node(cluster_outpipe_t *p) {
        ::serialize(p, master_store);
        ::serialize(p, master_get_store);
        ::serialize(p, registration_address);
        ::serialize(p, map_council_t<int, int>::address_t(&demo_map_council));
        ::serialize(p, routing_map.get_address());
    }

    class map_council_control_t : public control_t {
    private:
        map_council_t<int, int> *parent;
    public:
        map_council_control_t(std::string key, std::string help, map_council_t<int, int> *parent)
            : control_t(key, help), parent(parent)
        { }
        std::string call(int argc, char **argv) {
            if (argc == 3) {
                parent->insert(atoi(argv[1]), atoi(argv[2]));
                return std::string("Applied\n");
            } else {
                std::string res;
                char buffer[100];
                for (std::map<int, int>::const_iterator it = parent->get_value().begin(); it != parent->get_value().end(); it++) {
                    sprintf(buffer, "%d -> %d\n", it->first, it->second);
                    res.append(buffer);
                }
                return res;
            }
        }
    };
    map_council_control_t map_council_control;
};

struct cluster_config_t {
    /* We accept memcached connections on port (31400+id). We accept cluster connections on
    port (31000+id). Our database file is ("rethinkdb_data_%d" % id). */
    int id;
    int contact_id;   // -1 for a new cluster
};

void serve(int id, demo_delegate_t *delegate) {

    os_signal_cond_t os_signal_cond;

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

    cache_t::create(multiplexer.proxies[0], &config.store_static_config.cache);
    cache_t cache(multiplexer.proxies[0], &config.store_dynamic_config.cache);

    btree_slice_t::create(&cache);
    btree_slice_t slice(&cache, 1000);

    set_store_mailbox_t change_mailbox(&slice);
    get_store_mailbox_t get_mailbox(&slice);
    delegate->registration_address.call(get_cluster()->us, &change_mailbox, &get_mailbox);
    delegate->routing_map.master_map.insert(get_cluster()->us, set_store_mailbox_t::address_t(&change_mailbox));
    delegate->routing_map.storage_map.insert(get_cluster()->us, std::make_pair(&change_mailbox, &get_mailbox));

    for (routing::routing_map_t::master_map_t::const_iterator it = delegate->routing_map.master_map.begin(); it != delegate->routing_map.master_map.end(); it++) {
        logINF("Master map entry: %d\n", it->first);
    }

    int serve_port = 31400 + id;
    protocol_listener_t conn_acceptor(serve_port, &delegate->master_get_store, &delegate->master_store);
    logINF("Accepting connections on port %d\n", serve_port);

    wait_for_sigint();
}

void add_listener(int peer, clustered_store_t *dispatcher, set_store_mailbox_t::address_t set_addr, get_store_mailbox_t::address_t get_addr) {
    clustered_store_t::dispatchee_t dispatchee(peer, dispatcher, make_pair(&set_addr, &get_addr));
    struct 
        : public cond_t, public cluster_peer_t::kill_cb_t 
    {
        void on_kill() { pulse(); }
    } kill_waiter;
    kill_waiter.wait();
}

void cluster_main(cluster_config_t config, UNUSED thread_pool_t *thread_pool) {

    if (config.contact_id == -1) {

        /* Start a new cluster */

        /* TODO: We initially pass a `NULL` delegate and then later call `set_delegate()`. This is
        a hack and leads to race conditions because another node could start up and try to
        connect before we called `set_delegate()`. This is temporary and the metadata-cluster
        logic will make it obsolete. */

        logINF("Starting new cluster...\n");
        cluster_t the_cluster(31000 + config.id,
            NULL);
        logINF("Cluster started.\n");

        /* Start the master-components */

        clustered_store_t dispatcher;
        registration_mailbox_t registration_mailbox(boost::bind(&add_listener, _1, &dispatcher, _2, _3));

        timestamping_set_store_interface_t timestamper(&dispatcher);
        set_store_interface_mailbox_t master_mailbox(&timestamper);

        get_store_mailbox_t master_get_mailbox(&dispatcher);

        demo_delegate_t delegate(&master_mailbox, &master_get_mailbox, &registration_mailbox);
        the_cluster.set_delegate(&delegate);

        serve(config.id, static_cast<demo_delegate_t *>(the_cluster.get_delegate()));

    } else {

        /* Join an existing cluster */

        logINF("Joining an existing cluster.\n");
        cluster_t the_cluster(31000 + config.id, "localhost", 31000 + config.contact_id,
            &demo_delegate_t::construct);
        logINF("Cluster started.\n");

        serve(config.id, static_cast<demo_delegate_t *>(the_cluster.get_delegate()));
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
