// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/main/initial_join.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/wait_any.hpp"
#include "logger.hpp"

// This is a helper function used by find_peer_address_in_set to avoid code duplication.
// It will return an iterator to the first peer_address_t in the set that matches any
// of the IP addresses in addr.
peer_address_set_t::iterator find_peer_address_internal(const peer_address_set_t &peers,
                                                        const std::set<ip_and_port_t> &addrs) {
    for (peer_address_set_t::iterator other = peers.begin(); other != peers.end(); ++other) {
        for (auto it = addrs.begin(); it != addrs.end(); ++it) {
            for (auto jt = other->ips().begin();
                 jt != other->ips().end(); ++jt) {
                if (*it == *jt) {
                    return other;
                }
            }
        }
    }

    return peers.end();
}

// We need to make sure we find the *right* address in the set.
// Example:
//  Set contains [127.0.1.1] and [192.168.0.15].
//  find is called with [127.0.0.1, 127.0.1.1, 192.168.0.15].
// In this case, only 192.168.0.15 should be removed, because the loopback
// address is obviously talking about a different peer.
// So, first we create a peer_address_t without any loopback addresses
// then, if there are no matches for that, use the original.
peer_address_set_t::iterator find_peer_address_in_set(const peer_address_set_t &peers,
                                                      const peer_address_t &addr) {
    // Compare non-loopback addresses first
    const std::set<ip_and_port_t> &addrs = addr.ips();
    std::set<ip_and_port_t> addrs_no_loopback;
    for (std::set<ip_and_port_t>::const_iterator it = addrs.begin();
         it != addrs.end(); ++it) {
        if (!it->ip().is_loopback()) {
            addrs_no_loopback.insert(*it);
        }
    }

    peer_address_set_t::iterator result =
        find_peer_address_internal(peers, addrs_no_loopback);

    if (result == peers.end()) {
        // No match found, compare including loopback addresses
        result = find_peer_address_internal(peers, addr.ips());
    }

    return result;
}

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

    peer_address_t self_addr = cluster->get_peer_address(cluster->get_me());
    for (peer_address_set_t::iterator join_self_it = find_peer_address_in_set(peers_not_heard_from, self_addr);
         join_self_it != peers_not_heard_from.end();
         join_self_it = find_peer_address_in_set(peers_not_heard_from, self_addr)) {
        peers_not_heard_from.erase(*join_self_it);
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
static const double retry_interval_growth_rate = 1.5;
static const int grace_period_before_warn_ms = 1000 * 5;

void initial_joiner_t::main_coro(connectivity_cluster_t::run_t *cluster_run, auto_drainer_t::lock_t keepalive) {
    try {
        int retry_interval_ms = initial_retry_interval_ms;
        logINF("Attempting connection to %zu peer%s...",
               peers_not_heard_from.size(), peers_not_heard_from.size() == 1 ? "" : "s");
        do {
            for (peer_address_set_t::iterator it = peers_not_heard_from.begin(); it != peers_not_heard_from.end(); it++) {
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
            peer_address_set_t::iterator it = peers_not_heard_from.begin();
            printf_buffer_t buffer;
            debug_print(&buffer, it->primary_host());
            for (it++; it != peers_not_heard_from.end(); it++) {
                buffer.appendf(", ");
                debug_print(&buffer, it->primary_host());
            }
            logWRN("We were unable to connect to the following peer%s, or the --join address does not match the peer's canonical address: %s", peers_not_heard_from.size() > 1 ? "s" : "", buffer.c_str());
        }
    } catch (const interrupted_exc_t &) {
        /* ignore */
    }

    if (!done_signal.is_pulsed())
        done_signal.pulse();
}

void initial_joiner_t::on_connect(peer_id_t peer) {
    if (peer != cluster->get_me()) {
        successful_connection = true;
        peer_address_t peer_addr = cluster->get_peer_address(peer);

        // We want to remove a peer address, find it in the set (if it's there at all, and remove it)
        peer_address_set_t::iterator join_addr = find_peer_address_in_set(peers_not_heard_from, peer_addr);
        if (join_addr != peers_not_heard_from.end()) {
            peers_not_heard_from.erase(*join_addr);
        }

        if (!done_signal.is_pulsed()) {
            done_signal.pulse();

            if (!grace_period_timer.has()) {
                grace_period_timer.init(new signal_timer_t(grace_period_before_warn_ms));
            }
        }
    }
}

