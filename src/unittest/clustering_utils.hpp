#ifndef UNITTEST_CLUSTERING_UTILS_HPP_
#define UNITTEST_CLUSTERING_UTILS_HPP_

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"
#include "rpc/directory/view.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

using mock::dummy_protocol_t;
using mock::a_thru_z_region;

template<class protocol_t>
class test_store_t {
public:
    test_store_t() :
            temp_file("/tmp/rdb_unittest.XXXXXX"),
            store(temp_file.name(), true)
    {
        /* Initialize store metadata */
        cond_t non_interruptor;
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> token;
        store.new_write_token(token);
        region_map_t<protocol_t, binary_blob_t> new_metainfo(
                store.get_region(),
                binary_blob_t(version_range_t(version_t::zero()))
            );
        store.set_metainfo(new_metainfo, token, &non_interruptor);
    }

    temp_file_t temp_file;
    typename protocol_t::store_t store;
};

inline void test_inserter_write_namespace_if(namespace_interface_t<dummy_protocol_t> *nif, const std::string& key, const std::string& value, order_token_t otok, signal_t *interruptor) {
    dummy_protocol_t::write_t w;
    w.values[key] = value;
    nif->write(w, otok, interruptor);
}

inline std::string test_inserter_read_namespace_if(namespace_interface_t<dummy_protocol_t> *nif, const std::string& key, order_token_t otok, signal_t *interruptor) {
    dummy_protocol_t::read_t r;
    r.keys.keys.insert(key);
    dummy_protocol_t::read_response_t resp = nif->read(r, otok, interruptor);
    return resp.values[key];
}

class test_inserter_t {

public:
    typedef std::map<std::string, std::string> state_t;

    test_inserter_t(boost::function<void(const std::string&, const std::string&, order_token_t, signal_t *)> _wfun, 
               boost::function<std::string(const std::string&, order_token_t, signal_t *)> _rfun,
               boost::function<std::string()> _key_gen_fun,
               order_source_t *_osource, state_t *state)
        : values_inserted(state), drainer(new auto_drainer_t), wfun(_wfun), rfun(_rfun), key_gen_fun(_key_gen_fun), osource(_osource)
    {
        coro_t::spawn_sometime(boost::bind(&test_inserter_t::insert_forever,
            this, osource, auto_drainer_t::lock_t(drainer.get())));
    }

    template<class protocol_t>
    test_inserter_t(namespace_interface_t<protocol_t> *namespace_if, boost::function<std::string()> _key_gen_fun, order_source_t *_osource, state_t *state)
        : values_inserted(state),
          drainer(new auto_drainer_t), 
          wfun(boost::bind(&test_inserter_t::write_namespace_if<protocol_t>, namespace_if, _1, _2, _3, _4)),
          rfun(boost::bind(&test_inserter_t::read_namespace_if<protocol_t>, namespace_if, _1, _2, _3)),
          key_gen_fun(_key_gen_fun),
          osource(_osource)
    {
        coro_t::spawn_sometime(boost::bind(&test_inserter_t::insert_forever,
            this, osource, auto_drainer_t::lock_t(drainer.get())));
    }

    void stop() {
        rassert(drainer);
        drainer.reset();
    }

    state_t *values_inserted;

private:
    template<class protocol_t>
    static void write_namespace_if(namespace_interface_t<protocol_t> *namespace_if, const std::string& key, const std::string& value, order_token_t otok, signal_t *interruptor) {
        test_inserter_write_namespace_if(namespace_if, key, value, otok, interruptor);
    }

    template<class protocol_t>
    static std::string read_namespace_if(namespace_interface_t<protocol_t> *namespace_if, const std::string& key, order_token_t otok, signal_t *interruptor) {
        return test_inserter_read_namespace_if(namespace_if, key, otok, interruptor);
    }

    boost::scoped_ptr<auto_drainer_t> drainer;

    void insert_forever(
            order_source_t *osource,
            auto_drainer_t::lock_t keepalive) {
        try {
            for (int i = 0; ; i++) {
                if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();

                dummy_protocol_t::write_t w;
                std::string key = key_gen_fun();
                std::string value = (*values_inserted)[key] = strprintf("%d", i);

                cond_t interruptor;
                wfun(key, value, osource->check_in("unittest"), &interruptor);

                nap(10, keepalive.get_drain_signal());
            }
        } catch (interrupted_exc_t) {
            /* Break out of loop */
        }
    }

public:
    void validate() {
        for (state_t::iterator it = values_inserted->begin();
                               it != values_inserted->end(); 
                               it++) {
            cond_t interruptor;
            std::string response = rfun((*it).first, osource->check_in("unittest"), &interruptor);
            EXPECT_EQ((*it).second, response);
        }
    }

private:
    boost::function<void(const std::string&, const std::string&, order_token_t, signal_t *)> wfun;
    boost::function<std::string(const std::string&, order_token_t, signal_t *)> rfun;
    boost::function<std::string()> key_gen_fun;
    order_source_t *osource;
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
inline std::string key_gen<dummy_protocol_t>() {
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
        connectivity_cluster_run(&connectivity_cluster, 10000 + randint(20000), &mailbox_manager)
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

template<class metadata_t>
class simple_directory_manager_t : public directory_readwrite_service_t {
public:
    simple_directory_manager_t(simple_mailbox_cluster_t *p, const metadata_t &md) :
        parent(p), metadata(md)
        { }
    ~simple_directory_manager_t() THROWS_NOTHING { }
    clone_ptr_t<directory_rwview_t<metadata_t> > get_root_view() {
        return clone_ptr_t<root_view_t>(new root_view_t(this));
    }
    connectivity_service_t *get_connectivity_service() {
        return parent->get_connectivity_service();
    }
private:
    class root_view_t : public directory_rwview_t<metadata_t> {
    public:
        explicit root_view_t(simple_directory_manager_t *p) : parent(p) { }
        root_view_t *clone() const THROWS_NOTHING {
            return new root_view_t(parent);
        }
        boost::optional<metadata_t> get_value(peer_id_t peer) THROWS_NOTHING {
            parent->assert_thread();
            if (peer == parent->parent->get_connectivity_service()->get_me()) {
                return boost::optional<metadata_t>(parent->metadata);
            } else {
                return boost::optional<metadata_t>();
            }
        }
        metadata_t get_our_value(directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
            parent->assert_thread();
            proof->assert_is_holding(parent);
            return parent->metadata;
        }
        void set_our_value(const metadata_t &new_value_for_us, directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
            parent->assert_thread();
            proof->assert_is_holding(parent);
            rwi_lock_assertion_t::write_acq_t acq(&parent->metadata_rwi_lock_assertion);
            parent->metadata = new_value_for_us;
            parent->metadata_publisher.publish(&simple_directory_manager_t::call_function_with_no_args);
        }
        directory_readwrite_service_t *get_directory_service() THROWS_NOTHING {
            return parent;
        }
    private:
        simple_directory_manager_t *parent;
    };
    static void call_function_with_no_args(const boost::function<void()> &f) {
        f();
    }
    rwi_lock_assertion_t *get_peer_value_lock(peer_id_t p) THROWS_NOTHING {
        assert_thread();
        rassert(p == parent->get_connectivity_service()->get_me());
        return &metadata_rwi_lock_assertion;
    }
    publisher_t<boost::function<void()> > *get_peer_value_publisher(peer_id_t p, peer_value_freeze_t *proof) THROWS_NOTHING {
        assert_thread();
        proof->assert_is_holding(this, p);
        rassert(p == parent->get_connectivity_service()->get_me());
        return metadata_publisher.get_publisher();
    }
    mutex_t *get_our_value_lock() THROWS_NOTHING {
        assert_thread();
        return &our_value_lock;
    }
    simple_mailbox_cluster_t *parent;
    metadata_t metadata;
    mutex_t our_value_lock;
    rwi_lock_assertion_t metadata_rwi_lock_assertion;
    publisher_controller_t<boost::function<void()> > metadata_publisher;
};

}   /* namespace unittest */

#endif /* UNITTEST_CLUSTERING_UTILS_HPP_ */
