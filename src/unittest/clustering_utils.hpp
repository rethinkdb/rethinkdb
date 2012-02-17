#ifndef __UNITTEST_CLUSTERING_UTILS_HPP__
#define __UNITTEST_CLUSTERING_UTILS_HPP__

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "rpc/directory/view.hpp"
#include "unittest/dummy_protocol.hpp"

namespace unittest {

class test_store_t {
public:
    test_store_t() :
        underlying_store(a_thru_z_region()),
        store(&underlying_store, a_thru_z_region())
    {
        /* Initialize store metadata */
        cond_t non_interruptor;
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> token;
        store.new_write_token(token);
        region_map_t<dummy_protocol_t, binary_blob_t> new_metainfo(
                a_thru_z_region(),
                binary_blob_t(version_range_t(version_t::zero()))
            );
        store.set_metainfo(new_metainfo, token, &non_interruptor);
    }

    dummy_underlying_store_t underlying_store;
    dummy_store_view_t store;
};

class inserter_t {

public:
    inserter_t(boost::function<dummy_protocol_t::write_response_t(dummy_protocol_t::write_t, order_token_t, signal_t *)> fun, order_source_t *osource) :
        drainer(new auto_drainer_t)
    {
        coro_t::spawn_sometime(boost::bind(&inserter_t::insert_forever,
            this, fun, osource, auto_drainer_t::lock_t(drainer.get())));
    }

    void stop() {
        rassert(drainer);
        drainer.reset();
    }

    std::map<std::string, std::string> values_inserted;

private:
    boost::scoped_ptr<auto_drainer_t> drainer;

    void insert_forever(
            boost::function<dummy_protocol_t::write_response_t(dummy_protocol_t::write_t, order_token_t, signal_t *)> fun,
            order_source_t *osource,
            auto_drainer_t::lock_t keepalive) {
        try {
            for (int i = 0; ; i++) {

                if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();

                dummy_protocol_t::write_t w;
                std::string key = std::string(1, 'a' + rand() % 26);
                w.values[key] = values_inserted[key] = strprintf("%d", i);

                cond_t interruptor;
                fun(w, osource->check_in("unittest"), &interruptor);

                nap(10, keepalive.get_drain_signal());
            }
        } catch (interrupted_exc_t) {
            /* Break out of loop */
        }
    }
};

class simple_mailbox_cluster_t {
public:
    simple_mailbox_cluster_t() :
        mailbox_manager(&connectivity_cluster),
        connectivity_cluster_run(&connectivity_cluster, 10000 + rand() % 20000, &mailbox_manager)
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

#endif /* __UNITTEST_CLUSTERING_UTILS_HPP__ */
