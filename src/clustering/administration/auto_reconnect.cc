#include "clustering/administration/auto_reconnect.hpp"

#include "arch/timing.hpp"
#include "concurrency/wait_any.hpp"

auto_reconnector_t::auto_reconnector_t(
        connectivity_cluster_t *connectivity_cluster_,
        connectivity_cluster_t::run_t *connectivity_cluster_run_,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &machine_id_translation_table_,
        const boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > &machine_metadata_) :
    connectivity_cluster(connectivity_cluster_),
    connectivity_cluster_run(connectivity_cluster_run_),
    machine_id_translation_table(machine_id_translation_table_),
    machine_metadata(machine_metadata_),
    machine_id_translation_table_subs(boost::bind(&auto_reconnector_t::on_connect_or_disconnect, this))
{
    watchable_t<std::map<peer_id_t, machine_id_t> >::freeze_t freeze(machine_id_translation_table);
    machine_id_translation_table_subs.reset(machine_id_translation_table, &freeze);
    on_connect_or_disconnect();
}

void auto_reconnector_t::on_connect_or_disconnect() {
    std::map<peer_id_t, machine_id_t> map = machine_id_translation_table->get();
    for (std::map<peer_id_t, machine_id_t>::iterator it = map.begin(); it != map.end(); ++it) {
        if (connected_peers.find(it->first) == connected_peers.end()) {
            connected_peers.insert(std::make_pair(
                it->first,
                std::make_pair(
                    it->second,
                    connectivity_cluster->get_peer_address(it->first)
                )));
        }
    }
    for (std::map<peer_id_t, std::pair<machine_id_t, peer_address_t> >::iterator it = connected_peers.begin();
            it != connected_peers.end();) {
        if (map.find(it->first) == map.end()) {
            std::map<peer_id_t, std::pair<machine_id_t, peer_address_t> >::iterator jt = it;
            ++jt;
            coro_t::spawn_sometime(boost::bind(
                &auto_reconnector_t::try_reconnect, this,
                it->second.first, it->second.second, auto_drainer_t::lock_t(&drainer)
                ));
            connected_peers.erase(it);
            it = jt;
        } else {
            ++it;
        }
    }
}

static const int initial_backoff_ms = 50;
static const int max_backoff_ms = 1000 * 15;
static const double backoff_growth_rate = 1.5;

void auto_reconnector_t::try_reconnect(machine_id_t machine, peer_address_t last_known_address, auto_drainer_t::lock_t keepalive) {

    cond_t cond;
    semilattice_read_view_t<machines_semilattice_metadata_t>::subscription_t subs(
        boost::bind(&auto_reconnector_t::pulse_if_machine_declared_dead, this, machine, &cond),
        machine_metadata);
    pulse_if_machine_declared_dead(machine, &cond);
    watchable_t<std::map<peer_id_t, machine_id_t> >::subscription_t subs2(
        boost::bind(&auto_reconnector_t::pulse_if_machine_reconnected, this, machine, &cond));
    {
        watchable_t<std::map<peer_id_t, machine_id_t> >::freeze_t freeze(machine_id_translation_table);
        subs2.reset(machine_id_translation_table, &freeze);
        pulse_if_machine_reconnected(machine, &cond);
    }

    wait_any_t interruptor(&cond, keepalive.get_drain_signal());

    int backoff_ms = initial_backoff_ms;
    try {
        while (true) {
            connectivity_cluster_run->join(last_known_address);
            signal_timer_t timer(backoff_ms);
            wait_interruptible(&timer, &interruptor);
            rassert(backoff_ms * backoff_growth_rate > backoff_ms, "rounding screwed it up");
            backoff_ms = std::min(static_cast<int>(backoff_ms * backoff_growth_rate), max_backoff_ms);
        }
    } catch (interrupted_exc_t) {
        /* ignore; this is how we escape the loop */
    }
}

void auto_reconnector_t::pulse_if_machine_declared_dead(machine_id_t machine, cond_t *c) {
    machines_semilattice_metadata_t mmd = machine_metadata->get();
    machines_semilattice_metadata_t::machine_map_t::iterator it = mmd.machines.find(machine);
    if (it == mmd.machines.end()) {
        /* The only way we could have gotten here is if a machine connected
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

void auto_reconnector_t::pulse_if_machine_reconnected(machine_id_t machine, cond_t *c) {
    std::map<peer_id_t, machine_id_t> map = machine_id_translation_table->get();
    for (std::map<peer_id_t, machine_id_t>::iterator it = map.begin(); it != map.end(); ++it) {
        if (it->second == machine) {
            if (!c->is_pulsed()) {
                c->pulse();
            }
            break;
        }
    }
}

