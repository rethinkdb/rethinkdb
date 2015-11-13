// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef UNITTEST_CLUSTERING_UTILS_HPP_
#define UNITTEST_CLUSTERING_UTILS_HPP_

#include <functional>
#include <map>
#include <set>
#include <string>

#include "arch/io/disk.hpp"
#include "arch/timing.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/remote_replicator_metadata.hpp"
#include "clustering/immediate_consistency/remote_replicator_server.hpp"
#include "clustering/query_routing/primary_query_client.hpp"
#include "clustering/query_routing/primary_query_server.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_store.hpp"
#include "unittest/unittest_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/store.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/merger.hpp"

namespace unittest {

class simple_write_callback_t :
    public primary_dispatcher_t::write_callback_t, public cond_t
{
public:
    simple_write_callback_t() : acks(0) { }
    write_durability_t get_default_write_durability() {
        return write_durability_t::HARD;
    }
    void on_ack(const server_id_t &, write_response_t &&) {
        ++acks;
    }
    void on_end() {
        EXPECT_GE(acks, 1);
        pulse();
    }
    int acks;
};

inline merger_serializer_t *create_and_construct_serializer(temp_file_t *temp_file, io_backender_t *io_backender) {
    filepath_file_opener_t file_opener(temp_file->name(), io_backender);
    log_serializer_t::create(&file_opener,
                                  log_serializer_t::static_config_t());
    auto inner_serializer = make_scoped<log_serializer_t>(
            log_serializer_t::dynamic_config_t(),
            &file_opener,
            &get_global_perfmon_collection());
    return new merger_serializer_t(std::move(inner_serializer),
                                   MERGER_SERIALIZER_MAX_ACTIVE_WRITES);
}

class test_store_t {
public:
    test_store_t(io_backender_t *io_backender, order_source_t *order_source, rdb_context_t *ctx) :
            serializer(create_and_construct_serializer(&temp_file, io_backender)),
            balancer(new dummy_cache_balancer_t(GIGABYTE)),
            store(region_t::universe(), serializer.get(), balancer.get(),
                temp_file.name().permanent_path(), true,
                &get_global_perfmon_collection(), ctx, io_backender, base_path_t("."),
                generate_uuid(), update_sindexes_t::UPDATE) {
        /* Initialize store metadata */
        cond_t non_interruptor;
        write_token_t token;
        store.new_write_token(&token);
        region_map_t<binary_blob_t> new_metainfo(
                store.get_region(),
                binary_blob_t(version_t::zero()));
        store.set_metainfo(new_metainfo, order_source->check_in("test_store_t"), &token,
            write_durability_t::SOFT, &non_interruptor);
    }

    temp_file_t temp_file;
    scoped_ptr_t<merger_serializer_t> serializer;
    scoped_ptr_t<cache_balancer_t> balancer;
    store_t store;
};

class test_inserter_t {

public:
    typedef std::map<std::string, std::string> state_t;

    test_inserter_t(
            order_source_t *_osource,
            const std::string &_tag,
            state_t *state) :
        values_inserted(state), osource(_osource), tag(_tag), next_value(0)
    {
        for (const auto &pair : *values_inserted) {
            keys_used.push_back(pair.first);
        }
    }

    virtual ~test_inserter_t() {
        guarantee(!running(), "subclass should call stop()");
    }

    void insert(size_t n) {
        while (n-- > 0) {
            insert_one();
        }
    }

    void start() {
        drainer.init(new auto_drainer_t);
        coro_t::spawn_sometime(std::bind(
            &test_inserter_t::insert_forever, this,
            auto_drainer_t::lock_t(drainer.get())));
    }

    void stop() {
        rassert(drainer.has());
        drainer.reset();
    }

    bool running() const {
        return drainer.has();
    }

    void validate() {
        for (state_t::iterator it = values_inserted->begin();
                               it != values_inserted->end();
                               it++) {
            cond_t non_interruptor;
            std::string response = read(
                it->first,
                osource->check_in(strprintf("mock::test_inserter_t::validate(%p)", this))
                    .with_read_mode(),
                &non_interruptor);
            if (it->second != response) {
                report_error(it->first, it->second, response);
            }
        }
    }

    /* `validate_no_extras()` makes sure that no keys from `extras` are present that are
    not supposed to be present. It complements `validate`, which only checks for the
    existence of keys that are supposed to exist. */
    void validate_no_extras(const std::map<std::string, std::string> &extras) {
        for (const auto &pair : extras) {
            auto it = values_inserted->find(pair.first);
            std::string expect = it != values_inserted->end() ? it->second : "";
            cond_t non_interruptor;
            std::string actual = read(
                pair.first,
                osource->check_in(strprintf("mock::test_inserter_t::validate(%p)", this))
                    .with_read_mode(),
                &non_interruptor);
            if (expect != actual) {
                report_error(pair.first, expect, actual);
            }
        }
    }

    state_t *values_inserted;

protected:
    virtual void write(
            const std::string &, const std::string &, order_token_t, signal_t *) = 0;
    virtual std::string read(const std::string &, order_token_t, signal_t *) = 0;
    virtual std::string generate_key() = 0;

    virtual void report_error(
            const std::string &key,
            const std::string &expect,
            const std::string &actual) {
        crash("For key `%s`: expected `%s`, got `%s`\n",
            key.c_str(), expect.c_str(), actual.c_str());
    }

private:
    scoped_ptr_t<auto_drainer_t> drainer;

    void insert_one() {
        std::string key, value;
        if (randint(3) != 0 || keys_used.empty()) {
            key = generate_key();
            value = strprintf("%d", next_value++);
            keys_used.push_back(key);
        } else {
            key = keys_used.at(randint(keys_used.size()));
            if (randint(2) == 0) {
                /* `wfun()` may choose to interpret this as a deletion */
                value = "";
            } else {
                value = strprintf("%d", next_value++);
            }
        }
        (*values_inserted)[key] = value;
        cond_t interruptor;
        write(key, value, osource->check_in(tag), &interruptor);
    }

    void insert_forever(auto_drainer_t::lock_t keepalive) {
        try {
            for (;;) {
                if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();
                insert_one();
                nap(10, keepalive.get_drain_signal());
            }
        } catch (const interrupted_exc_t &) {
            /* Break out of loop */
        }
    }

    std::vector<std::string> keys_used;
    order_source_t *osource;
    std::string tag;
    int next_value;

    DISABLE_COPYING(test_inserter_t);
};

inline std::string alpha_key_gen(int len) {
    std::string key;
    for (int j = 0; j < len; j++) {
        key.push_back('a' + randint(26));
    }
    return key;
}

inline std::string dummy_key_gen() {
    return alpha_key_gen(1);
}

inline std::string mc_key_gen() {
    return alpha_key_gen(100);
}

peer_address_t get_cluster_local_address(connectivity_cluster_t *cm);

class simple_mailbox_cluster_t {
public:
    simple_mailbox_cluster_t() :
        server_id(generate_uuid()),
        mailbox_manager(&connectivity_cluster, 'M'),
        heartbeat_manager(heartbeat_semilattice_metadata),
        connectivity_cluster_run(&connectivity_cluster,
                                 server_id,
                                 get_unittest_addresses(),
                                 peer_address_t(),
                                 ANY_PORT,
                                 0,
                                 heartbeat_manager.get_view())
        { }
    connectivity_cluster_t *get_connectivity_cluster() {
        return &connectivity_cluster;
    }
    mailbox_manager_t *get_mailbox_manager() {
        return &mailbox_manager;
    }
    void connect(simple_mailbox_cluster_t *other) {
        connectivity_cluster_run.join(
            get_cluster_local_address(&other->connectivity_cluster));
    }
    void disconnect(simple_mailbox_cluster_t *other) {
        auto_drainer_t::lock_t keepalive;
        connectivity_cluster_t::connection_t *conn = connectivity_cluster.get_connection(
            other->connectivity_cluster.get_me(),
            &keepalive);
        guarantee(conn != nullptr);
        conn->kill_connection();
    }
private:
    connectivity_cluster_t connectivity_cluster;
    server_id_t server_id;
    mailbox_manager_t mailbox_manager;
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t> heartbeat_manager;
    connectivity_cluster_t::run_t connectivity_cluster_run;
};

}  // namespace unittest

#endif  // UNITTEST_CLUSTERING_UTILS_HPP_
