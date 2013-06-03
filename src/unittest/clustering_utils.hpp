// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UNITTEST_CLUSTERING_UTILS_HPP_
#define UNITTEST_CLUSTERING_UTILS_HPP_

#include <map>
#include <set>
#include <string>

#include "arch/io/disk.hpp"
#include "arch/timing.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/query/master_access.hpp"
#include "mock/dummy_protocol.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"
#include "serializer/config.hpp"

class memcached_protocol_t;

namespace unittest {

// An ack checker that only expects to get passed to spawn_write.
class spawn_write_fake_ack_checker_t : public ack_checker_t {
    bool is_acceptable_ack_set(const std::set<peer_id_t> &) {
        EXPECT_TRUE(false);
        return false;
    }
    write_durability_t get_write_durability(const peer_id_t &) const {
        return WRITE_DURABILITY_SOFT;
    }
};

struct fake_fifo_enforcement_t {
    fifo_enforcer_source_t source;
    fifo_enforcer_sink_t sink;
};

inline standard_serializer_t *create_and_construct_serializer(temp_file_t *temp_file, io_backender_t *io_backender) {
    filepath_file_opener_t file_opener(temp_file->name(), io_backender);
    standard_serializer_t::create(&file_opener,
                                  standard_serializer_t::static_config_t());
    return new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                     &file_opener,
                                     &get_global_perfmon_collection());
}

template<class protocol_t>
class test_store_t {
public:
    test_store_t(io_backender_t *io_backender, order_source_t *order_source, typename protocol_t::context_t *ctx) :
            serializer(create_and_construct_serializer(&temp_file, io_backender)),
            store(serializer.get(), temp_file.name().permanent_path(), GIGABYTE,
                    true, &get_global_perfmon_collection(), ctx, io_backender, base_path_t(".")) {
        /* Initialize store metadata */
        cond_t non_interruptor;
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> token;
        store.new_write_token(&token);
        region_map_t<protocol_t, binary_blob_t> new_metainfo(
                store.get_region(),
                binary_blob_t(version_range_t(version_t::zero())));
        store.set_metainfo(new_metainfo, order_source->check_in("test_store_t"), &token, &non_interruptor);
    }

    temp_file_t temp_file;
    scoped_ptr_t<standard_serializer_t> serializer;
    typename protocol_t::store_t store;
};

inline void test_inserter_write_master_access(master_access_t<mock::dummy_protocol_t> *ma, const std::string &key, const std::string &value, order_token_t otok, signal_t *interruptor) {
    mock::dummy_protocol_t::write_t w;
    mock::dummy_protocol_t::write_response_t response;
    w.values[key] = value;
    fifo_enforcer_sink_t::exit_write_t write_token;
    ma->new_write_token(&write_token);
    ma->write(w, &response, otok, &write_token, interruptor);
}

inline std::string test_inserter_read_master_access(master_access_t<mock::dummy_protocol_t> *ma, const std::string &key, order_token_t otok, signal_t *interruptor) {
    mock::dummy_protocol_t::read_t r;
    mock::dummy_protocol_t::read_response_t response;
    r.keys.keys.insert(key);
    fifo_enforcer_sink_t::exit_read_t read_token;
    ma->new_read_token(&read_token);
    ma->read(r, &response, otok, &read_token, interruptor);
    return response.values.find(key)->second;
}

inline void test_inserter_write_namespace_if(namespace_interface_t<mock::dummy_protocol_t> *nif, const std::string& key, const std::string& value, order_token_t otok, signal_t *interruptor) {
    mock::dummy_protocol_t::write_t w;
    mock::dummy_protocol_t::write_response_t response;
    w.values[key] = value;
    nif->write(w, &response, otok, interruptor);
}

inline std::string test_inserter_read_namespace_if(namespace_interface_t<mock::dummy_protocol_t> *nif, const std::string& key, order_token_t otok, signal_t *interruptor) {
    mock::dummy_protocol_t::read_t r;
    mock::dummy_protocol_t::read_response_t response;
    r.keys.keys.insert(key);
    nif->read(r, &response, otok, interruptor);
    return response.values[key];
}

class test_inserter_t {

public:
    typedef std::map<std::string, std::string> state_t;

    test_inserter_t(boost::function<void(const std::string&, const std::string&, order_token_t, signal_t *)> _wfun,
               boost::function<std::string(const std::string&, order_token_t, signal_t *)> _rfun,
               boost::function<std::string()> _key_gen_fun,
                    order_source_t *_osource, const std::string& tag, state_t *state)
        : values_inserted(state), drainer(new auto_drainer_t), wfun(_wfun), rfun(_rfun), key_gen_fun(_key_gen_fun), osource(_osource)
    {
        coro_t::spawn_sometime(boost::bind(&test_inserter_t::insert_forever,
                                           this, tag, auto_drainer_t::lock_t(drainer.get())));
    }

    template <class protocol_t>
    test_inserter_t(namespace_interface_t<protocol_t> *namespace_if, boost::function<std::string()> _key_gen_fun, order_source_t *_osource, const std::string& tag, state_t *state)
        : values_inserted(state),
          drainer(new auto_drainer_t),
          wfun(boost::bind(&test_inserter_t::write_namespace_if<protocol_t>, namespace_if, _1, _2, _3, _4)),
          rfun(boost::bind(&test_inserter_t::read_namespace_if<protocol_t>, namespace_if, _1, _2, _3)),
          key_gen_fun(_key_gen_fun),
          osource(_osource)
    {
        coro_t::spawn_sometime(boost::bind(&test_inserter_t::insert_forever,
                                           this, tag, auto_drainer_t::lock_t(drainer.get())));
    }

    template <class protocol_t>
    test_inserter_t(master_access_t<protocol_t> *master_access, boost::function<std::string()> _key_gen_fun, order_source_t *_osource, const std::string& tag, state_t *state)
        : values_inserted(state),
          drainer(new auto_drainer_t),
          wfun(boost::bind(&test_inserter_t::write_master_access<protocol_t>, master_access, _1, _2, _3, _4)),
          rfun(boost::bind(&test_inserter_t::read_master_access<protocol_t>, master_access, _1, _2, _3)),
          key_gen_fun(_key_gen_fun),
          osource(_osource)
    {
        coro_t::spawn_sometime(boost::bind(&test_inserter_t::insert_forever,
                                           this, tag, auto_drainer_t::lock_t(drainer.get())));
    }

    void stop() {
        rassert(drainer.has());
        drainer.reset();
    }

    state_t *values_inserted;

private:
    template<class protocol_t>
    static void write_master_access(master_access_t<protocol_t> *ma, const std::string& key, const std::string& value, order_token_t otok, signal_t *interruptor) {
        test_inserter_write_master_access(ma, key, value, otok, interruptor);
    }

    template<class protocol_t>
    static std::string read_master_access(master_access_t<protocol_t> *ma, const std::string& key, order_token_t otok, signal_t *interruptor) {
        return test_inserter_read_master_access(ma, key, otok, interruptor);
    }

    template<class protocol_t>
    static void write_namespace_if(namespace_interface_t<protocol_t> *namespace_if, const std::string& key, const std::string& value, order_token_t otok, signal_t *interruptor) {
        test_inserter_write_namespace_if(namespace_if, key, value, otok, interruptor);
    }

    template<class protocol_t>
    static std::string read_namespace_if(namespace_interface_t<protocol_t> *namespace_if, const std::string& key, order_token_t otok, signal_t *interruptor) {
        return test_inserter_read_namespace_if(namespace_if, key, otok, interruptor);
    }

    scoped_ptr_t<auto_drainer_t> drainer;

    void insert_forever(const std::string &msg, auto_drainer_t::lock_t keepalive) {
        try {
            std::string tag = strprintf("insert_forever(%p,%s)", this, msg.c_str());
            for (int i = 0; ; i++) {
                if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();

                mock::dummy_protocol_t::write_t w;
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
            rassert(it->second == response);
        }
    }

private:
    boost::function<void(const std::string&, const std::string&, order_token_t, signal_t *)> wfun;
    boost::function<std::string(const std::string&, order_token_t, signal_t *)> rfun;
    boost::function<std::string()> key_gen_fun;
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

template <class protocol_t>
std::string key_gen();

template <>
inline std::string key_gen<mock::dummy_protocol_t>() {
    return dummy_key_gen();
}

template <>
inline std::string key_gen<memcached_protocol_t>() {
    return mc_key_gen();
}

class simple_mailbox_cluster_t {
public:
    simple_mailbox_cluster_t() :
        mailbox_manager(&connectivity_cluster),
        connectivity_cluster_run(&connectivity_cluster,
                                 get_unittest_addresses(),
                                 ANY_PORT,
                                 &mailbox_manager,
                                 0,
                                 NULL)
        { }
    connectivity_service_t *get_connectivity_service() {
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
template <class protocol_t>
struct equality_metainfo_checker_callback_t : public metainfo_checker_callback_t<protocol_t> {
    explicit equality_metainfo_checker_callback_t(const binary_blob_t& expected_value)
        : value_(expected_value) { }

    void check_metainfo(const region_map_t<protocol_t, binary_blob_t>& metainfo, const typename protocol_t::region_t& region) const {
        region_map_t<protocol_t, binary_blob_t> masked = metainfo.mask(region);

        for (typename region_map_t<protocol_t, binary_blob_t>::const_iterator it = masked.begin(); it != masked.end(); ++it) {
            rassert(it->second == value_);
        }
    }

private:
    const binary_blob_t value_;

    DISABLE_COPYING(equality_metainfo_checker_callback_t);
};
#endif

}  // namespace unittest

#endif  // UNITTEST_CLUSTERING_UTILS_HPP_
