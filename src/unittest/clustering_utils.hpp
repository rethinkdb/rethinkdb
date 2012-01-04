#ifndef __UNITTEST_CLUSTERING_UTILS_HPP__
#define __UNITTEST_CLUSTERING_UTILS_HPP__

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "unittest/dummy_protocol.hpp"

namespace unittest {

class test_store_t {
public:
    test_store_t() :
        underlying_store(a_thru_z_region()),
        store(&underlying_store, a_thru_z_region())
    {
        /* Initialize store metadata */
        cond_t interruptor;
        store.begin_write_transaction(&interruptor)->set_metadata(
            region_map_t<dummy_protocol_t, binary_blob_t>(
                a_thru_z_region(),
                binary_blob_t(version_range_t(version_t::zero()))
            ));
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
    connectivity_cluster_t connectivity_cluster;
    mailbox_manager_t mailbox_manager;
    connectivity_cluster_t::run_t connectivity_cluster_run;
};

}   /* namespace unittest */

#endif /* __UNITTEST_CLUSTERING_UTILS_HPP__ */
