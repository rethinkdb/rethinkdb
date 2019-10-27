#include "rpc/mailbox/disconnect_watcher.hpp"

#include "rpc/connectivity/peer_id.hpp"
#include "rpc/mailbox/mailbox.hpp"

disconnect_watcher_t::disconnect_watcher_t(mailbox_manager_t *mailbox_manager,
                                           peer_id_t peer) {
    if (mailbox_manager->get_connectivity_cluster()->
            get_connection(peer, &connection_keepalive) != nullptr) {
        /* The peer is currently connected. Start watching for when they disconnect. */
        signal_t::subscription_t::reset(connection_keepalive.get_drain_signal());
    } else {
        /* The peer is not currently connected. Pulse ourself immediately. */
        pulse();
    }
}

/* This is the callback for when `connection_keepalive.get_drain_signal()` is pulsed */
void disconnect_watcher_t::run() {
    pulse();
}

