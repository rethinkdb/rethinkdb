// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/servers/auto_reconnect.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/timing.hpp"
#include "concurrency/wait_any.hpp"

auto_reconnector_t::auto_reconnector_t(
        connectivity_cluster_t *connectivity_cluster_,
        connectivity_cluster_t::run_t *connectivity_cluster_run_,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, server_id_t> > > &server_id_translation_table_,
        const boost::shared_ptr<semilattice_read_view_t<servers_semilattice_metadata_t> > &server_metadata_) :
    connectivity_cluster(connectivity_cluster_),
    connectivity_cluster_run(connectivity_cluster_run_),
    server_id_translation_table(server_id_translation_table_),
    server_metadata(server_metadata_),
    server_id_translation_table_subs(boost::bind(&auto_reconnector_t::on_connect_or_disconnect, this)),
    connection_subs(
        connectivity_cluster->get_connections(),
        std::bind(&auto_reconnector_t::on_connect_or_disconnect, this),
        initial_call_t::NO)
{
    watchable_t<change_tracking_map_t<peer_id_t, server_id_t> >::freeze_t freeze(
        server_id_translation_table);
    server_id_translation_table_subs.reset(server_id_translation_table, &freeze);
    on_connect_or_disconnect();
}

void auto_reconnector_t::on_connect_or_disconnect() {
    std::map<peer_id_t, server_id_t> map = server_id_translation_table->get().get_inner();
    for (std::map<peer_id_t, server_id_t>::iterator it = map.begin(); it != map.end(); it++) {
        if (server_ids.find(it->first) == server_ids.end()) {
            auto_drainer_t::lock_t connection_keepalive;
            connectivity_cluster_t::connection_t *connection =
                connectivity_cluster->get_connection(it->first, &connection_keepalive);
            if (connection == NULL) {
                /* This can happen due to a race condition in `watchable_t`. Treat the
                peer as if it's not actually connected yet, even though it has an entry
                in the directory. */
                continue;
            }
            server_ids.insert(std::make_pair(it->first, it->second));
            addresses[it->second] = connection->get_peer_address();
        }
    }
    for (auto it = server_ids.begin(); it != server_ids.end();) {
        if (map.find(it->first) == map.end()) {
            auto jt = it;
            ++jt;
            coro_t::spawn_sometime(boost::bind(
                &auto_reconnector_t::try_reconnect, this, it->second,
                auto_drainer_t::lock_t(&drainer)));
            server_ids.erase(it);
            it = jt;
        } else {
            it++;
        }
    }
}

static const int initial_backoff_ms = 50;
static const int max_backoff_ms = 1000 * 15;
static const double backoff_growth_rate = 1.5;

void auto_reconnector_t::try_reconnect(server_id_t server,
                                       auto_drainer_t::lock_t keepalive) {
    peer_address_t last_known_address;
    auto it = addresses.find(server);
    if (it == addresses.end()) {
        /* This can happen because of a race condition: the server was declared dead, so
        its entry was removed from the map before we got to it. */
        return;
    }
    last_known_address = it->second;

    cond_t declared_dead;
    semilattice_read_view_t<servers_semilattice_metadata_t>::subscription_t subs(
        boost::bind(&auto_reconnector_t::pulse_if_server_declared_dead,
            this, server, &declared_dead),
        server_metadata);
    pulse_if_server_declared_dead(server, &declared_dead);

    cond_t reconnected;
    watchable_t<change_tracking_map_t<peer_id_t, server_id_t> >::subscription_t subs2(
        boost::bind(&auto_reconnector_t::pulse_if_server_reconnected,
            this, server, &reconnected));
    {
        watchable_t<change_tracking_map_t<peer_id_t, server_id_t> >::freeze_t freeze(
            server_id_translation_table);
        subs2.reset(server_id_translation_table, &freeze);
        pulse_if_server_reconnected(server, &reconnected);
    }

    wait_any_t interruptor(&declared_dead, &reconnected, keepalive.get_drain_signal());

    int backoff_ms = initial_backoff_ms;
    try {
        while (true) {
            connectivity_cluster_run->join(last_known_address);
            signal_timer_t timer;
            timer.start(backoff_ms);
            wait_interruptible(&timer, &interruptor);
            guarantee(backoff_ms * backoff_growth_rate > backoff_ms, "rounding screwed it up");
            backoff_ms = std::min(static_cast<int>(backoff_ms * backoff_growth_rate), max_backoff_ms);
        }
    } catch (const interrupted_exc_t &) {
        /* ignore; this is how we escape the loop */
    }

    /* If the server was declared dead, clean up its entry in the `addresses` map */
    if (declared_dead.is_pulsed()) {
        addresses.erase(server);
    }
}

void auto_reconnector_t::pulse_if_server_declared_dead(server_id_t server, cond_t *c) {
    servers_semilattice_metadata_t mmd = server_metadata->get();
    servers_semilattice_metadata_t::server_map_t::iterator it = mmd.servers.find(server);
    if (it == mmd.servers.end()) {
        /* The only way we could have gotten here is if a server connected
        for the first time and then immediately disconnected, and its directory
        got through but its semilattices didn't. Don't try to reconnect to it
        because there's no way to declare it dead. */
        if (!c->is_pulsed()) {
            c->pulse();
        }
    } else if (it->second.is_deleted()) {
        if (!c->is_pulsed()) {
            c->pulse();
        }
    }
}

void auto_reconnector_t::pulse_if_server_reconnected(server_id_t server, cond_t *c) {
    std::map<peer_id_t, server_id_t> map = server_id_translation_table->get().get_inner();
    for (std::map<peer_id_t, server_id_t>::iterator it = map.begin(); it != map.end(); it++) {
        if (it->second == server) {
            if (!c->is_pulsed()) {
                c->pulse();
            }
            break;
        }
    }
}

