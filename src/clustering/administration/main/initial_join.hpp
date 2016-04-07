// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_INITIAL_JOIN_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_INITIAL_JOIN_HPP_

#include <set>

#include "arch/timing.hpp"
#include "rpc/connectivity/cluster.hpp"

/* `initial_joiner_t` is the class that `rethinkdb serve` and `rethinkdb admin`
use to handle the `--join` flag. It takes a list of addresses; it repeatedly
tries to connect to the addresses until at least one connection has been made.
At that point, it stops trying to connect to the other addresses and pulses
`get_ready_signal()`. If after a certain period of time we have still not
successfully connected to one of the other addresses, it calls `logWRN()` with
a warning message. */

class initial_joiner_t {
public:
    initial_joiner_t(
            connectivity_cluster_t *cluster,
            connectivity_cluster_t::run_t *cluster_run,
            const peer_address_set_t &peers,
            const int join_delay,
            int timeout = -1);

    signal_t *get_ready_signal() {
        return &done_signal;
    }

    bool get_success() {
        return successful_connection;
    }

private:
    void main_coro(connectivity_cluster_t::run_t *cluster_run,
                   auto_drainer_t::lock_t keepalive);
    void on_connection_change(
        const peer_id_t &peer,
        const connectivity_cluster_t::connection_pair_t *pair);

    connectivity_cluster_t *cluster;
    peer_address_set_t peers_not_heard_from;
    cond_t done_signal;
    signal_timer_t grace_period_timer;
    int join_delay_secs;
    auto_drainer_t drainer;
    watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t>
        ::all_subs_t subs;
    bool successful_connection;

    DISABLE_COPYING(initial_joiner_t);
};


#endif /* CLUSTERING_ADMINISTRATION_MAIN_INITIAL_JOIN_HPP_ */

