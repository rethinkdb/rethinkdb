// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef UNITTEST_CLUSTERING_UTILS_HPP_
#define UNITTEST_CLUSTERING_UTILS_HPP_

#include <functional>
#include <map>
#include <set>
#include <string>

#include "arch/io/disk.hpp"
#include "arch/timing.hpp"
#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/query/master_access.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_store.hpp"
#include "unittest/unittest_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/store.hpp"
#include "serializer/config.hpp"

namespace unittest {

class simple_write_callback_t : public broadcaster_t::write_callback_t, public cond_t {
public:
    simple_write_callback_t() : acks(0) { }
    write_durability_t get_default_write_durability() {
        return write_durability_t::HARD;
    }
    void on_ack(const server_id_t &, write_response_t &&) {
        ++acks;
    }
    void on_end() {
        EXPECT_EQ(1, acks);
        pulse();
    }
    int acks;
};

inline standard_serializer_t *create_and_construct_serializer(temp_file_t *temp_file, io_backender_t *io_backender) {
    filepath_file_opener_t file_opener(temp_file->name(), io_backender);
    standard_serializer_t::create(&file_opener,
                                  standard_serializer_t::static_config_t());
    return new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                     &file_opener,
                                     &get_global_perfmon_collection());
}

class test_store_t {
public:
    test_store_t(io_backender_t *io_backender, order_source_t *order_source, rdb_context_t *ctx) :
            serializer(create_and_construct_serializer(&temp_file, io_backender)),
            balancer(new dummy_cache_balancer_t(GIGABYTE)),
            store(serializer.get(), balancer.get(), temp_file.name().permanent_path(), true,
                  &get_global_perfmon_collection(), ctx, io_backender, base_path_t("."),
                  scoped_ptr_t<outdated_index_report_t>(), generate_uuid()) {
        /* Initialize store metadata */
        cond_t non_interruptor;
        write_token_t token;
        store.new_write_token(&token);
        region_map_t<binary_blob_t> new_metainfo(
                store.get_region(),
                binary_blob_t(version_t::zero()));
        store.set_metainfo(new_metainfo, order_source->check_in("test_store_t"), &token, &non_interruptor);
    }

    temp_file_t temp_file;
    scoped_ptr_t<standard_serializer_t> serializer;
    scoped_ptr_t<cache_balancer_t> balancer;
    store_t store;
};

inline void test_inserter_write_master_access(master_access_t *ma, const std::string &key, const std::string &value, order_token_t otok, signal_t *interruptor) {
    write_t w = mock_overwrite(key, value);
    write_response_t response;
    fifo_enforcer_sink_t::exit_write_t write_token;
    ma->new_write_token(&write_token);
    ma->write(w, &response, otok, &write_token, interruptor);
}

inline std::string test_inserter_read_master_access(master_access_t *ma, const std::string &key, order_token_t otok, signal_t *interruptor) {
    read_t r = mock_read(key);
    read_response_t response;
    fifo_enforcer_sink_t::exit_read_t read_token;
    ma->new_read_token(&read_token);
    ma->read(r, &response, otok, &read_token, interruptor);
    return mock_parse_read_response(response);
}

inline void test_inserter_write_namespace_if(namespace_interface_t *nif, const std::string &key, const std::string &value, order_token_t otok, signal_t *interruptor) {
    write_response_t response;
    nif->write(mock_overwrite(key, value), &response, otok, interruptor);
}

inline std::string test_inserter_read_namespace_if(namespace_interface_t *nif, const std::string &key, order_token_t otok, signal_t *interruptor) {
    read_response_t response;
    nif->read(mock_read(key), &response, otok, interruptor);
    return mock_parse_read_response(response);
}

class test_inserter_t {

public:
    typedef std::map<std::string, std::string> state_t;

    test_inserter_t(std::function<void(const std::string &, const std::string &, order_token_t, signal_t *)> _wfun,
               std::function<std::string(const std::string &, order_token_t, signal_t *)> _rfun,
               std::function<std::string()> _key_gen_fun,
                    order_source_t *_osource, const std::string& tag, state_t *state)
        : values_inserted(state), drainer(new auto_drainer_t), wfun(_wfun), rfun(_rfun), key_gen_fun(_key_gen_fun), osource(_osource)
    {
        coro_t::spawn_sometime(std::bind(&test_inserter_t::insert_forever,
                                         this, tag, auto_drainer_t::lock_t(drainer.get())));
    }

    test_inserter_t(namespace_interface_t *namespace_if, std::function<std::string()> _key_gen_fun, order_source_t *_osource, const std::string& tag, state_t *state)
        : values_inserted(state),
          drainer(new auto_drainer_t),
          wfun(std::bind(&test_inserter_t::write_namespace_if, namespace_if, ph::_1, ph::_2, ph::_3, ph::_4)),
          rfun(std::bind(&test_inserter_t::read_namespace_if, namespace_if, ph::_1, ph::_2, ph::_3)),
          key_gen_fun(_key_gen_fun),
          osource(_osource)
    {
        coro_t::spawn_sometime(std::bind(&test_inserter_t::insert_forever,
                                         this, tag, auto_drainer_t::lock_t(drainer.get())));
    }

    test_inserter_t(master_access_t *master_access, std::function<std::string()> _key_gen_fun, order_source_t *_osource, const std::string& tag, state_t *state)
        : values_inserted(state),
          drainer(new auto_drainer_t),
          wfun(std::bind(&test_inserter_t::write_master_access, master_access, ph::_1, ph::_2, ph::_3, ph::_4)),
          rfun(std::bind(&test_inserter_t::read_master_access, master_access, ph::_1, ph::_2, ph::_3)),
          key_gen_fun(_key_gen_fun),
          osource(_osource)
    {
        coro_t::spawn_sometime(std::bind(&test_inserter_t::insert_forever,
                                         this, tag, auto_drainer_t::lock_t(drainer.get())));
    }

    void stop() {
        rassert(drainer.has());
        drainer.reset();
    }

    state_t *values_inserted;

private:
    static void write_master_access(master_access_t *ma, const std::string& key, const std::string& value, order_token_t otok, signal_t *interruptor) {
        test_inserter_write_master_access(ma, key, value, otok, interruptor);
    }

    static std::string read_master_access(master_access_t *ma, const std::string& key, order_token_t otok, signal_t *interruptor) {
        return test_inserter_read_master_access(ma, key, otok, interruptor);
    }

    static void write_namespace_if(namespace_interface_t *namespace_if, const std::string& key, const std::string& value, order_token_t otok, signal_t *interruptor) {
        test_inserter_write_namespace_if(namespace_if, key, value, otok, interruptor);
    }

    static std::string read_namespace_if(namespace_interface_t *namespace_if, const std::string& key, order_token_t otok, signal_t *interruptor) {
        return test_inserter_read_namespace_if(namespace_if, key, otok, interruptor);
    }

    scoped_ptr_t<auto_drainer_t> drainer;

    void insert_forever(const std::string &msg, auto_drainer_t::lock_t keepalive) {
        try {
            std::string tag = strprintf("insert_forever(%p,%s)", this, msg.c_str());
            for (int i = 0; ; i++) {
                if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();

                std::string key = key_gen_fun();
                std::string value = (*values_inserted)[key] = strprintf("%d", i);

                cond_t interruptor;
                wfun(key, value, osource->check_in(tag), &interruptor);

                nap(10, keepalive.get_drain_signal());
            }
        } catch (const interrupted_exc_t &) {
            /* Break out of loop */
        }
    }

public:
    void validate() {
        for (state_t::iterator it = values_inserted->begin();
                               it != values_inserted->end();
                               it++) {
            cond_t interruptor;
            std::string response = rfun(it->first, osource->check_in(strprintf("mock::test_inserter_t::validate(%p)", this)).with_read_mode(), &interruptor);
            guarantee(it->second == response, "For key `%s`: expected `%s`, got `%s`\n",
                it->first.c_str(), it->second.c_str(), response.c_str());
        }
    }

private:
    std::function<void(const std::string &, const std::string &, order_token_t, signal_t *)> wfun;
    std::function<std::string(const std::string &, order_token_t, signal_t *)> rfun;
    std::function<std::string()> key_gen_fun;
    order_source_t *osource;

    DISABLE_COPYING(test_inserter_t);
};

inline std::string dummy_key_gen() {
    return std::string(1, 'a' + randint(26));
}

inline std::string mc_key_gen() {
    std::string key;
    for (int j = 0; j < 100; j++) {
        key.push_back('a' + randint(26));
    }
    return key;
}

class simple_mailbox_cluster_t {
public:
    simple_mailbox_cluster_t() :
        mailbox_manager(&connectivity_cluster, 'M'),
        connectivity_cluster_run(&connectivity_cluster,
                                 get_unittest_addresses(),
                                 peer_address_t(),
                                 ANY_PORT,
                                 0)
        { }
    connectivity_cluster_t *get_connectivity_cluster() {
        return &connectivity_cluster;
    }
    mailbox_manager_t *get_mailbox_manager() {
        return &mailbox_manager;
    }
private:
    connectivity_cluster_t connectivity_cluster;
    mailbox_manager_t mailbox_manager;
    connectivity_cluster_t::run_t connectivity_cluster_run;
};

#ifndef NDEBUG
struct equality_metainfo_checker_callback_t : public metainfo_checker_callback_t {
    explicit equality_metainfo_checker_callback_t(const binary_blob_t& expected_value)
        : value_(expected_value) { }

    void check_metainfo(const region_map_t<binary_blob_t>& metainfo, const region_t& region) const {
        region_map_t<binary_blob_t> masked = metainfo.mask(region);

        for (region_map_t<binary_blob_t>::const_iterator it = masked.begin(); it != masked.end(); ++it) {
            rassert(it->second == value_);
        }
    }

private:
    const binary_blob_t value_;

    DISABLE_COPYING(equality_metainfo_checker_callback_t);
};
#endif

peer_address_t get_cluster_local_address(connectivity_cluster_t *cm);

}  // namespace unittest

#endif  // UNITTEST_CLUSTERING_UTILS_HPP_
