// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_READ_MANAGER_TCC_
#define RPC_DIRECTORY_READ_MANAGER_TCC_

#include "rpc/directory/read_manager.hpp"

#include <map>
#include <utility>

#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/wait_any.hpp"
#include "config/args.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/versioned.hpp"
#include "stl_utils.hpp"

template<class metadata_t>
directory_read_manager_t<metadata_t>::directory_read_manager_t(
        connectivity_cluster_t *cm, connectivity_cluster_t::message_tag_t _tag)
        THROWS_NOTHING :
    cluster_message_handler_t(cm, _tag),
    variable(change_tracking_map_t<peer_id_t, metadata_t>())
{
    guarantee(get_connectivity_cluster()->get_connections()->get_all().empty());
}

template<class metadata_t>
directory_read_manager_t<metadata_t>::~directory_read_manager_t() THROWS_NOTHING {
    guarantee(get_connectivity_cluster()->get_connections()->get_all().empty());
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::on_message(
        connectivity_cluster_t::connection_t *connection,
        auto_drainer_t::lock_t connection_keepalive,
        read_stream_t *s)
        THROWS_ONLY(fake_archive_exc_t) {

    with_priority_t p(CORO_PRIORITY_DIRECTORY_CHANGES);

    uint8_t code = 0;
    {
        // All cluster versions use the uint8_t code here.
        archive_result_t res = deserialize_universal(s, &code);
        if (res != archive_result_t::SUCCESS) { throw fake_archive_exc_t(); }
    }

    guarantee(code == 'I' || code == 'U', "Invalid directory update code");

    switch (code) {
        case 'I': {
            /* Initial message from another peer */
            boost::shared_ptr<metadata_t> initial_value(new metadata_t());
            fifo_enforcer_state_t metadata_fifo_state;
            {
                archive_result_t res =
                    deserialize<cluster_version_t::CLUSTER>(s, initial_value.get());
                if (res != archive_result_t::SUCCESS) { throw fake_archive_exc_t(); }
                res = deserialize<cluster_version_t::CLUSTER>(s, &metadata_fifo_state);
                if (res != archive_result_t::SUCCESS) { throw fake_archive_exc_t(); }
            }

            /* Spawn a new coroutine because we might not be on the home thread
            and `on_message()` isn't supposed to block very long */
            coro_t::spawn_sometime(std::bind(
                &directory_read_manager_t::handle_connection, this,
                connection, connection_keepalive,
                initial_value, metadata_fifo_state,
                auto_drainer_t::lock_t(per_thread_drainers.get())));

            break;
        }

        case 'U': {
            /* Update from another peer */
            boost::shared_ptr<metadata_t> new_value(new metadata_t());
            fifo_enforcer_write_token_t metadata_fifo_token;
            {
                archive_result_t res =
                    deserialize<cluster_version_t::CLUSTER>(s, new_value.get());
                if (res != archive_result_t::SUCCESS) { throw fake_archive_exc_t(); }
                res = deserialize<cluster_version_t::CLUSTER>(s, &metadata_fifo_token);
                if (res != archive_result_t::SUCCESS) { throw fake_archive_exc_t(); }
            }

            /* Spawn a new coroutine because we might not be on the home thread
            and `on_message()` isn't supposed to block very long */
            coro_t::spawn_sometime(std::bind(
                &directory_read_manager_t::propagate_update, this,
                connection, connection_keepalive,
                new_value, metadata_fifo_token,
                auto_drainer_t::lock_t(per_thread_drainers.get())));

            break;
        }

        default: unreachable();
    }
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::handle_connection(
        connectivity_cluster_t::connection_t *connection,
        auto_drainer_t::lock_t connection_keepalive,
        const boost::shared_ptr<metadata_t> &new_value,
        fifo_enforcer_state_t metadata_fifo_state,
        auto_drainer_t::lock_t per_thread_keepalive)
        THROWS_NOTHING
{
    per_thread_keepalive.assert_is_holding(per_thread_drainers.get());
    wait_any_t interruptor(connection_keepalive.get_drain_signal(),
                           per_thread_keepalive.get_drain_signal());
    cross_thread_signal_t interruptor2(&interruptor, home_thread());
    on_thread_t thread_switcher(home_thread());

    mutex_assertion_t::acq_t mutex_assertion_lock(&mutex_assertion);

    /* Insert the initial value into the directory */
    variable.apply_atomic_op(
        [&](change_tracking_map_t<peer_id_t, metadata_t> *map) -> bool {
            map->begin_version();
            map->set_value(connection->get_peer_id(), *new_value);
            return true;
        });
    map_variable.set_key_no_equals(connection->get_peer_id(), std::move(*new_value));

    {
        fifo_enforcer_sink_t fifo_sink(metadata_fifo_state);
        connection_info_t connection_info;
        connection_info.fifo_sink = &fifo_sink;
        {
            map_insertion_sentry_t<connectivity_cluster_t::connection_t *,
                                   connection_info_t *>
                connection_info_insertion
                    (&connection_map, connection, &connection_info);

            for (auto it = waiting_for_initialization.lower_bound(connection);
                      it != waiting_for_initialization.upper_bound(connection);
                      it++) {
                it->second->pulse();
            }

            mutex_assertion_lock.reset();
            interruptor2.wait();
            mutex_assertion_lock.reset(&mutex_assertion);

            /* The `connection_info_insertion` destructor will run, so no further
            instances of `propagate_update` can grab the `auto_drainer_t` in
            `connection_info`. If any instances of `propagate_update` come along after
            this, they will put a `cond_t *` in `waiting_for_initialization` and then
            block until they realize that the connection has been closed. */
        }
        /* We don't want to be holding `mutex_assertion_lock` while the `auto_drainer_t`
        is being destroyed. */
        mutex_assertion_lock.reset();

        /* This will block until all instances of `propagate_update` are finished with
        `fifo_sink`. After this point, no instances of `propagate_update` for this
        connection can touch `variable`. */
        connection_info.drainer.drain();

        /* Now it's safe to delete `fifo_sink` and the directory entry. */
    }

    /* Delete the directory entry */
    variable.apply_atomic_op(
        [&](change_tracking_map_t<peer_id_t, metadata_t> *map) -> bool {
            map->begin_version();
            map->delete_value(connection->get_peer_id());
            return true;
        });
    map_variable.delete_key(connection->get_peer_id());
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_update(
        connectivity_cluster_t::connection_t *connection,
        auto_drainer_t::lock_t connection_keepalive,
        const boost::shared_ptr<metadata_t> &new_value,
        fifo_enforcer_write_token_t metadata_fifo_token,
        auto_drainer_t::lock_t per_thread_keepalive)
        THROWS_NOTHING
{
    per_thread_keepalive.assert_is_holding(per_thread_drainers.get());
    wait_any_t interruptor(connection_keepalive.get_drain_signal(),
                           per_thread_keepalive.get_drain_signal());
    cross_thread_signal_t interruptor2(&interruptor, home_thread());
    on_thread_t thread_switcher(home_thread());

    try {
        connection_info_t *connection_info;
        auto_drainer_t::lock_t connection_info_keepalive;

        {
            mutex_assertion_t::acq_t mutex_assertion_lock(&mutex_assertion);
            auto it = connection_map.find(connection);
            if (it == connection_map.end()) {
                /* There are two possibilities: either we reached this thread before the
                initialization message did, or the connection was just closed and we
                haven't found out yet. */
                cond_t wait_for_initialization;
                multimap_insertion_sentry_t<connectivity_cluster_t::connection_t *,
                                            cond_t *>
                    wait_insertion(
                        &waiting_for_initialization,
                        connection,
                        &wait_for_initialization);
                mutex_assertion_lock.reset();
                /* If we reached this thread before the initialization message did, then
                `wait_for_initialization` will be pulsed when the initialization message
                arrives. If the connection was closed, then `interruptor2` will be pulsed
                as soon as the news makes its way to this thread. */
                wait_interruptible(&wait_for_initialization, &interruptor2);
                mutex_assertion_lock.reset(&mutex_assertion);
                it = connection_map.find(connection);
                guarantee(it != connection_map.end());
            }
            connection_info = it->second;
            connection_info_keepalive =
                auto_drainer_t::lock_t(&connection_info->drainer);
        }

        /* Wait until it's our turn to update the directory. This ensures that updates
        aren't ever reordered; it would be bad if an old update overwrote a newer one. */
        fifo_enforcer_sink_t::exit_write_t exit_write(connection_info->fifo_sink,
                                                      metadata_fifo_token);
        wait_interruptible(&exit_write, &interruptor2);

        // This yield is here to avoid heartbeat timeouts in the following scenario:
        //  1. Have a cluster of many nodes, e.g. 64
        //  2. Create a table
        //  3. Reshard the table to 32 shards
        coro_t::yield();

        DEBUG_VAR mutex_assertion_t::acq_t mutex_assertion_lock(&mutex_assertion);
        variable.apply_atomic_op(
            [&](change_tracking_map_t<peer_id_t, metadata_t> *map) -> bool {
                map->begin_version();
                map->set_value(connection->get_peer_id(), *new_value);
                return true;
            });
        map_variable.set_key_no_equals(connection->get_peer_id(), std::move(*new_value));

    } catch (interrupted_exc_t) {
        /* This can only occur if we are shutting down or the connection was lost. In
        either case, it's safe to ignore the update. */
    }
}

#endif  // RPC_DIRECTORY_READ_MANAGER_TCC_
