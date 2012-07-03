#include "rpc/connectivity/connectivity.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

connectivity_service_t::peers_list_freeze_t::peers_list_freeze_t(connectivity_service_t *connectivity) :
    acq(connectivity->get_peers_list_lock()) { }

void connectivity_service_t::peers_list_freeze_t::assert_is_holding(connectivity_service_t *connectivity) {
    acq.assert_is_holding(connectivity->get_peers_list_lock());
}

connectivity_service_t::peers_list_subscription_t::peers_list_subscription_t(const peers_list_callback_pair_t &connect_disconnect_pair)
     : subs(connect_disconnect_pair) { }

void connectivity_service_t::peers_list_subscription_t::reset() {
    subs.reset();
}

void connectivity_service_t::peers_list_subscription_t::reset(connectivity_service_t *connectivity, peers_list_freeze_t *proof) {
    proof->assert_is_holding(connectivity);
    subs.reset(connectivity->get_peers_list_publisher());
}

disconnect_watcher_t::disconnect_watcher_t(connectivity_service_t *connectivity, peer_id_t p) :
    subs(peers_list_callback_pair_t(0, boost::bind(&disconnect_watcher_t::on_disconnect, this, _1))), peer(p) {
    ASSERT_FINITE_CORO_WAITING;
    connectivity_service_t::peers_list_freeze_t freeze(connectivity);
    if (!connectivity->get_peer_connected(peer)) {
        pulse();
    } else {
        subs.reset(connectivity, &freeze);
    }
}

void disconnect_watcher_t::on_disconnect(peer_id_t p) {
    if (peer == p) {
        if (!is_pulsed()) {
            pulse();
        }
    }
}
