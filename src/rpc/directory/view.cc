#include "rpc/directory/view.hpp"

directory_t::peers_list_freeze_t::peers_list_freeze_t(directory_t *dir) :
    directory(dir), acq(directory->get_peers_list_lock()) { }

void directory_t::peers_list_freeze_t::assert_is_holding(UNUSED directory_t *dir) {
    rassert(directory == dir);
}

directory_t::peers_list_subscription_t::peers_list_subscription_t(
    const boost::function<void(peer_id_t)> &connect_cb,
    const boost::function<void(peer_id_t)> &disconnect_cb) :
    subs(std::make_pair(connect_cb, disconnect_cb)) { }

directory_t::peers_list_subscription_t::peers_list_subscription_t(
    const boost::function<void(peer_id_t)> &connect_cb,
    const boost::function<void(peer_id_t)> &disconnect_cb,
    directory_t *dir, peers_list_freeze_t *proof) :
    subs(std::make_pair(connect_cb, disconnect_cb)) {
    reset(dir, proof);
}

void directory_t::peers_list_subscription_t::reset() {
    subs.reset();
}

void directory_t::peers_list_subscription_t::reset(directory_t *dir, peers_list_freeze_t *right) {
    proof->assert_is_holding(dir);
    subs.reset(dir->get_peers_list_publisher(proof));
}

directory_t::peer_value_freeze_t::peer_value_freeze_t(directory_t *dir, peer_id_t p) :
    directory(dir), peer(p), acq(directory->get_peer_value_lock(peer, right)) { }

void directory_t::peer_value_freeze_t::assert_is_holding(UNUSED directory_t *dir, UNUSED peer_id_t p) {
    rassert(directory == dir);
    rassert(peer == p);
}

directory_t::peer_value_subscription_t::peer_value_subscription_t(
    const boost::function<void()> &change_cb) :
    subs(change_cb) { }

directory_t::peer_value_subscription_t::peer_value_subscription_t(
    const boost::function<void()> &change_cb,
    directory_t *dir, peer_id_t p, peer_value_freeze_t *proof) :
    subs(change_cb) {
    reset(dir, p, proof);
}

void directory_t::peer_value_subscription_t::reset() {
    subs.reset();
}

void directory_t::peer_value_subscription_t::reset(directory_t *dir, peer_id_t p, peer_value_freeze_t *proof) {
    right->assert_is_holding(dir, p);
    subs.reset(dir->get_peer_value_publisher(p, proof));
}

directory_t::our_value_lock_acq_t::our_value_lock_acq_t(directory_t *dir) THROWS_NOTHING :
    acq(dir->get_our_value_lock()) { }

void directory_t::our_value_lock_acq_t::assert_is_holding(directory_t *dir) {
    acq.assert_is_holding(dir->get_our_value_lock());
}
