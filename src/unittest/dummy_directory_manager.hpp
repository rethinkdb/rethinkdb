// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef UNITTEST_DUMMY_DIRECTORY_MANAGER_HPP_
#define UNITTEST_DUMMY_DIRECTORY_MANAGER_HPP_

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "concurrency/watchable.hpp"
#include "containers/incremental_lenses.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/single_value_producer.hpp"

class dummy_directory_manager_value_t { };

template<class metadata_t>
class dummy_directory_manager_t :
    public coro_pool_callback_t<dummy_directory_manager_value_t> {
public:
    dummy_directory_manager_t(
                const peer_id_t &_local_peer_id,
                clone_ptr_t<watchable_t<metadata_t> > _local_entry) :
            local_peer_id(_local_peer_id),
            update_pool(1, &update_queue, this),
            local_entry(_local_entry),
            local_entry_subs(
                [&] () -> void {
                    update_queue.give_value(dummy_directory_manager_value_t());
                }),
            variable(change_tracking_map_t<peer_id_t, metadata_t>()) {
        watchable_freeze_t<metadata_t> freeze(local_entry);
        set_value_internal(local_peer_id, local_entry->get());
        local_entry_subs.reset(local_entry, &freeze);
    }

    ~dummy_directory_manager_t() { }

    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, metadata_t> > >
    get_view() THROWS_NOTHING {
        return variable.get_watchable();
    }

    void set_peer_value(const peer_id_t &peer, const metadata_t &new_value) {
        ASSERT_NE(peer, local_peer_id);
        set_value_internal(peer, new_value);
    }

    void delete_peer(const peer_id_t &peer) {
        ASSERT_NE(peer, local_peer_id);
        variable.apply_atomic_op(
            [&] (change_tracking_map_t<peer_id_t, metadata_t> *inner_map) -> bool {
                inner_map->begin_version();
                inner_map->delete_value(peer);
                return true;
            });
    }

private:
    void set_value_internal(const peer_id_t &peer, const metadata_t &new_value) {
        mutex_t::acq_t acq(&mutex);
        variable.apply_atomic_op(
            [&] (change_tracking_map_t<peer_id_t, metadata_t> *inner_map) -> bool {
                inner_map->begin_version();
                inner_map->set_value(peer, new_value);
                return true;
            });
    }

    void coro_pool_callback(UNUSED dummy_directory_manager_value_t dummy,
                            UNUSED signal_t *interruptor) {
        set_value_internal(local_peer_id, local_entry->get());
    }

    mutex_t mutex;
    peer_id_t local_peer_id;
    single_value_producer_t<dummy_directory_manager_value_t> update_queue;
    coro_pool_t<dummy_directory_manager_value_t> update_pool;
    clone_ptr_t<watchable_t<metadata_t> > local_entry;
    watchable_subscription_t<metadata_t> local_entry_subs;
    watchable_variable_t<change_tracking_map_t<peer_id_t, metadata_t> > variable;
};

#endif /* UNITTEST_DUMMY_DIRECTORY_MANAGER_HPP_ */
