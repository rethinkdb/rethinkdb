#include "rpc/connectivity/connectivity.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

connectivity_t::peers_list_freeze_t::peers_list_freeze_t(connectivity_t *connectivity) :
    acq(connectivity->get_peers_list_lock()) { }

void connectivity_t::peers_list_freeze_t::assert_is_holding(connectivity_t *connectivity) {
    acq.assert_is_holding(connectivity->get_peers_list_lock());
}

connectivity_t::peers_list_subscription_t::peers_list_subscription_t(
        const boost::function<void(peer_id_t)> &on_connect,
        const boost::function<void(peer_id_t)> &on_disconnect) :
    subs(std::make_pair(on_connect, on_disconnect)) { }

connectivity_t::peers_list_subscription_t::peers_list_subscription_t(
        const boost::function<void(peer_id_t)> &on_connect,
        const boost::function<void(peer_id_t)> &on_disconnect,
        connectivity_t *connectivity, peers_list_freeze_t *proof) :
    subs(std::make_pair(on_connect, on_disconnect)) {
    reset(connectivity, proof);
}

void connectivity_t::peers_list_subscription_t::reset() {
    subs.reset();
}

void connectivity_t::peers_list_subscription_t::reset(connectivity_t *connectivity, peers_list_freeze_t *proof) {
    proof->assert_is_holding(connectivity);
    subs.reset(connectivity->get_peers_list_publisher());
}

disconnect_watcher_t::disconnect_watcher_t(connectivity_t *connectivity, peer_id_t p) :
    subs(0, boost::bind(&disconnect_watcher_t::on_disconnect, this, _1)), peer(p) {
    connectivity_t::peers_list_freeze_t freeze(connectivity);
    if (connectivity->get_peer_connected(peer) == 0) {
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
