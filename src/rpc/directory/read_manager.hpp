// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_READ_MANAGER_HPP_
#define RPC_DIRECTORY_READ_MANAGER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/one_per_thread.hpp"
#include "concurrency/watchable.hpp"
#include "concurrency/watchable_map.hpp"
#include "containers/scoped.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "containers/incremental_lenses.hpp"

template<class metadata_t>
class directory_read_manager_t :
    public home_thread_mixin_t,
    public cluster_message_handler_t {
public:
    directory_read_manager_t(connectivity_cluster_t *cm,
            connectivity_cluster_t::message_tag_t tag) THROWS_NOTHING;
    ~directory_read_manager_t() THROWS_NOTHING;

    watchable_map_t<peer_id_t, metadata_t> *get_root_map_view() {
        return &map_variable;
    }

    /* This is deprecated. Use `get_root_map_view()` instead. */
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, metadata_t> > >
    get_root_view() THROWS_NOTHING {
        return variable.get_watchable();
    }

private:
    /* Note that connection and initialization are different things. Connection
    means that we can send messages to the peer. Initialization means that we
    have received initial metadata from the peer. Peers only show up in the
    `watchable_t` once initialization is complete. */

    /* This will be called by `connectivity_cluster_t`. It mustn't block. */
    void on_message(
            connectivity_cluster_t::connection_t *connection,
            auto_drainer_t::lock_t connection_keepalive,
            read_stream_t *stream)
            THROWS_ONLY(fake_archive_exc_t);

    /* `on_message()` will spawn `handle_connection()` in a new coroutine in response to
    the initialization message, and `propagate_update()` in response to all messages
    after that.

    They assume ownership of `new_value`. Semantically, the argument here is `metadata_t
    &&new_value` but we cannot easily pass that through to the coroutine call, which is
    why we use `boost::shared_ptr` instead. */
    void handle_connection(
            connectivity_cluster_t::connection_t *connection,
            auto_drainer_t::lock_t connection_keepalive,
            const boost::shared_ptr<metadata_t> &new_value,
            fifo_enforcer_state_t metadata_fifo_state,
            auto_drainer_t::lock_t per_thread_keepalive)
            THROWS_NOTHING;

    void propagate_update(
            connectivity_cluster_t::connection_t *connection,
            auto_drainer_t::lock_t connection_keepalive,
            const boost::shared_ptr<metadata_t> &new_value,
            fifo_enforcer_write_token_t metadata_fifo_token,
            auto_drainer_t::lock_t per_thread_keepalive)
            THROWS_NOTHING;

    watchable_variable_t<change_tracking_map_t<peer_id_t, metadata_t> > variable;
    watchable_map_var_t<peer_id_t, metadata_t> map_variable;

    class connection_info_t {
    public:
        fifo_enforcer_sink_t *fifo_sink;

        /* Destruction order is important here; the `auto_drainer_t` must be destroyed
        before the `fifo_sink` pointer goes out of scope. */
        auto_drainer_t drainer;
    };
    std::map<connectivity_cluster_t::connection_t *, connection_info_t *> connection_map;

    /* It's possible that messages can be reordered so that a `propagate_update()`
    reaches the home thread before the corresponding `handle_connection`. If this
    happens, the `propagate_update()` will put an entry in `waiting_for_initialization`
    and wait for `handle_connection()` to signal it. */
    std::multimap<connectivity_cluster_t::connection_t *, cond_t *>
        waiting_for_initialization;

    /* This protects `variable`, `fifo_sinks`, and `waiting_for_initialization` */
    mutex_assertion_t mutex_assertion;

    /* Instances of `propagate_initialization()` and `propagate_update()` hold
    a lock on one of these drainers. */
    one_per_thread_t<auto_drainer_t> per_thread_drainers;
};

#endif /* RPC_DIRECTORY_READ_MANAGER_HPP_ */
