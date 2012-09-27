#include "clustering/administration/main/initial_join.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/wait_any.hpp"
#include "logger.hpp"

initial_joiner_t::initial_joiner_t(
        connectivity_cluster_t *cluster_,
        connectivity_cluster_t::run_t *cluster_run,
        const peer_address_set_t &peers,
        int timeout_ms) :
    cluster(cluster_),
    peers_not_heard_from(peers),
    subs(this),
    successful_connection(false) {
    guarantee(!peers.empty());

    if (peers_not_heard_from.erase(cluster->get_peer_address(cluster->get_me())) != 0) {
        logWRN("Attempted to join self, peer ignored");
    }

    if (timeout_ms != -1) {
        grace_period_timer.init(new signal_timer_t(timeout_ms));
    }

    coro_t::spawn_sometime(boost::bind(&initial_joiner_t::main_coro, this, cluster_run, auto_drainer_t::lock_t(&drainer)));

    connectivity_service_t::peers_list_freeze_t freeze(cluster);
    subs.reset(cluster, &freeze);
    std::set<peer_id_t> already_connected = cluster->get_peers_list();
    for (std::set<peer_id_t>::iterator it = already_connected.begin(); it != already_connected.end(); it++) {
        on_connect(*it);
    }
}

static const int initial_retry_interval_ms = 200;
static const int max_retry_interval_ms = 1000 * 15;
static const float retry_interval_growth_rate = 1.5;
static const int grace_period_before_warn_ms = 1000 * 5;

void initial_joiner_t::main_coro(connectivity_cluster_t::run_t *cluster_run, auto_drainer_t::lock_t keepalive) {
    try {
        int retry_interval_ms = initial_retry_interval_ms;
        do {
            for (peer_address_set_t::const_iterator it = peers_not_heard_from.begin(); it != peers_not_heard_from.end(); it++) {
                cluster_run->join(*it);
            }
            signal_timer_t retry_timer(retry_interval_ms);
            wait_any_t waiter(&retry_timer);
            if (grace_period_timer.has()) {
                waiter.add(grace_period_timer.get());
            }
            wait_interruptible(&waiter, keepalive.get_drain_signal());
            retry_interval_ms = std::min(static_cast<int>(retry_interval_ms * retry_interval_growth_rate), max_retry_interval_ms);
        } while (!peers_not_heard_from.empty() && (!grace_period_timer.has() || !grace_period_timer->is_pulsed()));
        if (!peers_not_heard_from.empty()) {
            peer_address_set_t::const_iterator it = peers_not_heard_from.begin();
            std::string s = strprintf("%s:%d", it->primary_ip().as_dotted_decimal().c_str(), it->port);
            for (it++; it != peers_not_heard_from.end(); it++) {
                s += strprintf(", %s:%d", it->primary_ip().as_dotted_decimal().c_str(), it->port);
            }
            logWRN("We were unable to connect to the following peer(s): %s", s.c_str());
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }

    if (!done_signal.is_pulsed())
        done_signal.pulse();
}

void initial_joiner_t::on_connect(peer_id_t peer) {
    if (peer != cluster->get_me()) {
        successful_connection = true;
        peers_not_heard_from.erase(cluster->get_peer_address(peer));
        if (!done_signal.is_pulsed()) {
            done_signal.pulse();

            if (!grace_period_timer.has()) {
                grace_period_timer.init(new signal_timer_t(grace_period_before_warn_ms));
            }
        }
    }
}

