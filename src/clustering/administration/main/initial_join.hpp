#ifndef CLUSTERING_ADMINISTRATION_MAIN_INITIAL_JOIN_
#define CLUSTERING_ADMINISTRATION_MAIN_INITIAL_JOIN_

#include "arch/timing.hpp"
#include "rpc/connectivity/cluster.hpp"

/* `initial_joiner_t` is the class that `rethinkdb serve` and `rethinkdb admin`
use to handle the `--join` flag. It takes a list of addresses; it repeatedly
tries to connect to the addresses until at least one connection has been made.
At that point, it stops trying to connect to the other addresses and pulses
`get_ready_signal()`. If after a certain period of time we have still not
successfully connected to one of the other addresses, it calls `logWRN()` with
a warning message. */

struct initial_joiner_t {
public:
    initial_joiner_t(
            connectivity_cluster_t *cluster,
            connectivity_cluster_t::run_t *cluster_run,
            const std::set<peer_address_t> &peers);

    signal_t *get_ready_signal() {
        return &some_connection_made;
    }

private:
    void main_coro(connectivity_cluster_t::run_t *cluster_run, auto_drainer_t::lock_t keepalive);
    void on_connect(peer_id_t peer);

    connectivity_cluster_t *cluster;
    std::set<peer_address_t> peers_not_heard_from;
    cond_t some_connection_made;
    boost::scoped_ptr<signal_timer_t> grace_period_timer;
    auto_drainer_t drainer;
    connectivity_service_t::peers_list_subscription_t subs;

    DISABLE_COPYING(initial_joiner_t);
};


#endif /* CLUSTERING_ADMINISTRATION_MAIN_INITIAL_JOIN_ */

